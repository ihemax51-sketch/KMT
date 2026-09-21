#include "HWIDGenerator.h"
#include <Windows.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <iomanip>
#include <wincrypt.h>
#include <algorithm>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "crypt32.lib")

namespace {
    const char* HWID_PROTOCOL_MARKER = "KMT2";
    const char* HWID_KEY_CONTAINER = "KMTGuard.HWID.v2";
    const char* HWID_KEY_INIT_MUTEX = "Local\\KMTGuard.HWID.v2.KeyInit";
    const DWORD HWID_KEY_INIT_TIMEOUT_MS = 15000;
    const DWORD HWID_RSA_KEY_FLAGS = (2048 << 16);
    std::string g_hwidSessionNonce;

    void TraceHwidCryptoFailure(const char* operation, DWORD error) {
        char buffer[256] = { 0 };
        sprintf_s(buffer, sizeof(buffer),
                  "[KMTGuardKit][HWID] %s failed with Win32 error 0x%08lX.\n",
                  operation, error);
        OutputDebugStringA(buffer);
    }

    bool AcquireHwidKeyContainer(HCRYPTPROV& provider) {
        if (CryptAcquireContextA(&provider, HWID_KEY_CONTAINER,
                                 MS_ENH_RSA_AES_PROV_A, PROV_RSA_AES, 0))
            return true;

        DWORD error = GetLastError();
        if (error != NTE_BAD_KEYSET) {
            TraceHwidCryptoFailure("CryptAcquireContext(open)", error);
            return false;
        }

        if (CryptAcquireContextA(&provider, HWID_KEY_CONTAINER,
                                 MS_ENH_RSA_AES_PROV_A, PROV_RSA_AES,
                                 CRYPT_NEWKEYSET))
            return true;

        error = GetLastError();
        // Another client may have created the per-user container first.
        if (error == NTE_EXISTS &&
            CryptAcquireContextA(&provider, HWID_KEY_CONTAINER,
                                 MS_ENH_RSA_AES_PROV_A, PROV_RSA_AES, 0))
            return true;

        TraceHwidCryptoFailure("CryptAcquireContext(create)", error);
        return false;
    }

    bool RecreateEmptyHwidKeyContainer(HCRYPTPROV& provider,
                                       HCRYPTKEY& key) {
        if (provider != 0) {
            CryptReleaseContext(provider, 0);
            provider = 0;
        }

        HCRYPTPROV deleteContext = 0;
        if (!CryptAcquireContextA(&deleteContext, HWID_KEY_CONTAINER,
                                  MS_ENH_RSA_AES_PROV_A, PROV_RSA_AES,
                                  CRYPT_DELETEKEYSET)) {
            const DWORD deleteError = GetLastError();
            if (deleteError != NTE_BAD_KEYSET) {
                TraceHwidCryptoFailure("CryptAcquireContext(delete empty container)",
                                       deleteError);
                return false;
            }
        }

        if (!AcquireHwidKeyContainer(provider))
            return false;

        // A second process can complete initialization between retries.
        if (CryptGetUserKey(provider, AT_SIGNATURE, &key))
            return true;

        DWORD error = GetLastError();
        if (error != NTE_NO_KEY) {
            TraceHwidCryptoFailure("CryptGetUserKey(after recreate)", error);
            return false;
        }

        if (CryptGenKey(provider, AT_SIGNATURE, HWID_RSA_KEY_FLAGS, &key))
            return true;

        TraceHwidCryptoFailure("CryptGenKey(after recreate)", GetLastError());
        return false;
    }
}

std::string HWIDGenerator::GenerateHWID() {
    std::string hwid = "";

    // Sabit bilgileri topla
    std::string biosInfo = GetBIOSInfo();
    std::string diskInfo = GetDiskInfo();
    std::string cpuInfo = GetCPUInfo();

    // Donanım bilgilerini birleştir
    hwid = biosInfo + "|" + diskInfo + "|" + cpuInfo;
    return Sha256Hex(hwid);
}

std::string HWIDGenerator::BuildChallengeResponse(const std::string& challenge) {
    const std::string hwid = GenerateHWID();
    std::vector<std::string> fields;
    std::string current;
    for (std::string::size_type i = 0; i <= challenge.size(); ++i) {
        if (i == challenge.size() || challenge[i] == '|') {
            fields.push_back(current);
            current.clear();
        } else {
            current.push_back(challenge[i]);
        }
    }

    if (fields.size() != 4 || fields[0] != HWID_PROTOCOL_MARKER)
        return std::string(HWID_PROTOCOL_MARKER) + "|||";

    g_hwidSessionNonce = fields[3];

    const std::string canonical = fields[0] + "\n" + fields[1] + "\n" +
                                  fields[2] + "\n" + fields[3] + "\n" + hwid;
    std::vector<unsigned char> publicKey;
    std::vector<unsigned char> signature;
    if (!SignChallenge(canonical, publicKey, signature))
        return std::string(HWID_PROTOCOL_MARKER) + "|||";

    return std::string(HWID_PROTOCOL_MARKER) + "|" + hwid + "|" +
           Base64Encode(publicKey) + "|" + Base64Encode(signature);
}

std::string HWIDGenerator::GetSessionNonce() {
    return g_hwidSessionNonce;
}

std::string HWIDGenerator::Base64Encode(const std::vector<unsigned char>& value) {
    if (value.empty())
        return std::string();

    DWORD outputLength = 0;
    if (!CryptBinaryToStringA(&value[0], static_cast<DWORD>(value.size()),
                              CRYPT_STRING_BASE64,
                              NULL, &outputLength))
        return std::string();

    std::vector<char> output(outputLength, 0);
    if (!CryptBinaryToStringA(&value[0], static_cast<DWORD>(value.size()),
                              CRYPT_STRING_BASE64,
                              &output[0], &outputLength))
        return std::string();

    if (outputLength > 0 && output[outputLength - 1] == '\0')
        --outputLength;
    std::string encoded(&output[0], outputLength);
    encoded.erase(std::remove(encoded.begin(), encoded.end(), '\r'), encoded.end());
    encoded.erase(std::remove(encoded.begin(), encoded.end(), '\n'), encoded.end());
    return encoded;
}

bool HWIDGenerator::SignChallenge(const std::string& canonical,
                                  std::vector<unsigned char>& publicKey,
                                  std::vector<unsigned char>& signature) {
    HCRYPTPROV provider = 0;
    HCRYPTKEY key = 0;
    HCRYPTHASH hash = 0;
    HANDLE initializationMutex = 0;
    bool ownsInitializationMutex = false;
    bool success = false;

    initializationMutex = CreateMutexA(NULL, FALSE, HWID_KEY_INIT_MUTEX);
    if (initializationMutex != 0) {
        const DWORD waitResult = WaitForSingleObject(
            initializationMutex, HWID_KEY_INIT_TIMEOUT_MS);
        ownsInitializationMutex =
            waitResult == WAIT_OBJECT_0 || waitResult == WAIT_ABANDONED;
        if (!ownsInitializationMutex) {
            TraceHwidCryptoFailure("WaitForSingleObject(key initialization)",
                                   waitResult == WAIT_TIMEOUT ? WAIT_TIMEOUT : GetLastError());
            goto cleanup;
        }
    } else {
        TraceHwidCryptoFailure("CreateMutex(key initialization)", GetLastError());
    }

    if (!AcquireHwidKeyContainer(provider))
        goto cleanup;

    if (!CryptGetUserKey(provider, AT_SIGNATURE, &key)) {
        const DWORD getKeyError = GetLastError();
        if (getKeyError != NTE_NO_KEY) {
            TraceHwidCryptoFailure("CryptGetUserKey", getKeyError);
            goto cleanup;
        }

        if (!CryptGenKey(provider, AT_SIGNATURE, HWID_RSA_KEY_FLAGS, &key)) {
            TraceHwidCryptoFailure("CryptGenKey", GetLastError());
            // An interrupted first launch can leave a container with no
            // signature key. Only that proven-empty state is reset; an
            // existing valid device key is never deleted or replaced.
            if (!RecreateEmptyHwidKeyContainer(provider, key))
                goto cleanup;
        }
    }

    {
        DWORD publicKeyLength = 0;
        if (!CryptExportKey(key, 0, PUBLICKEYBLOB, 0, NULL, &publicKeyLength))
            goto cleanup;
        publicKey.resize(publicKeyLength);
        if (!CryptExportKey(key, 0, PUBLICKEYBLOB, 0, &publicKey[0], &publicKeyLength))
            goto cleanup;
        publicKey.resize(publicKeyLength);
    }

    if (!CryptCreateHash(provider, CALG_SHA_256, 0, 0, &hash) ||
        !CryptHashData(hash,
                       reinterpret_cast<const BYTE*>(canonical.data()),
                       static_cast<DWORD>(canonical.size()), 0))
        goto cleanup;

    {
        DWORD signatureLength = 0;
        if (!CryptSignHashA(hash, AT_SIGNATURE, NULL, 0, NULL, &signatureLength))
            goto cleanup;
        signature.resize(signatureLength);
        if (!CryptSignHashA(hash, AT_SIGNATURE, NULL, 0,
                            &signature[0], &signatureLength))
            goto cleanup;
        signature.resize(signatureLength);
        std::reverse(signature.begin(), signature.end());
    }

    success = true;

cleanup:
    if (hash != 0)
        CryptDestroyHash(hash);
    if (key != 0)
        CryptDestroyKey(key);
    if (provider != 0)
        CryptReleaseContext(provider, 0);
    if (ownsInitializationMutex)
        ReleaseMutex(initializationMutex);
    if (initializationMutex != 0)
        CloseHandle(initializationMutex);
    if (!success) {
        publicKey.clear();
        signature.clear();
    }
    return success;
}

std::string HWIDGenerator::Sha256Hex(const std::string& value) {
    HCRYPTPROV provider = 0;
    HCRYPTHASH hash = 0;
    BYTE digest[32] = { 0 };
    DWORD digestLength = sizeof(digest);
    std::ostringstream output;

    if (!CryptAcquireContext(&provider, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
        return std::string(64, '0');

    if (!CryptCreateHash(provider, CALG_SHA_256, 0, 0, &hash)) {
        CryptReleaseContext(provider, 0);
        return std::string(64, '0');
    }

    const BYTE* bytes = reinterpret_cast<const BYTE*>(value.data());
    if (!CryptHashData(hash, bytes, static_cast<DWORD>(value.size()), 0) ||
        !CryptGetHashParam(hash, HP_HASHVAL, digest, &digestLength, 0)) {
        CryptDestroyHash(hash);
        CryptReleaseContext(provider, 0);
        return std::string(64, '0');
    }

    output << std::uppercase << std::hex << std::setfill('0');
    for (DWORD i = 0; i < digestLength; ++i)
        output << std::setw(2) << static_cast<unsigned int>(digest[i]);

    CryptDestroyHash(hash);
    CryptReleaseContext(provider, 0);
    return output.str();
}

std::string HWIDGenerator::GetBIOSInfo() {
    const DWORD size = GetSystemFirmwareTable('RSMB', 0, NULL, 0);
    if (size == 0)
        return "NO_BIOS";

    // BIOS Seri Numarasını al
    std::vector<BYTE> data(size);
    if (GetSystemFirmwareTable('RSMB', 0, &data[0], size) != size)
        return "NO_BIOS";

    std::ostringstream output;
    output << std::uppercase << std::hex << std::setfill('0');
    for (DWORD i = 0; i < size; ++i)
        output << std::setw(2) << static_cast<unsigned int>(data[i]);
    return output.str();
}

std::string HWIDGenerator::GetDiskInfo() {
    std::string diskInfo = "";

    // Sabit Disk Seri Numarasını al
    DWORD dwVolumeSerialNumber = 0;
    if (!GetVolumeInformationA("C:\\", NULL, 0, &dwVolumeSerialNumber, NULL, NULL, NULL, 0))
        return "NO_DISK";
    std::stringstream ss;
    ss << std::hex << dwVolumeSerialNumber;
    diskInfo = ss.str();

    return diskInfo;
}

std::string HWIDGenerator::GetCPUInfo() {
    std::string cpuInfo = "";

    unsigned long s1 = 0;
    unsigned long s2 = 0;
    unsigned long s3 = 0;
    unsigned long s4 = 0;
    __asm
    {
    mov eax, 00h
    xor edx, edx
    cpuid
    mov s1, edx
    mov s2, eax
    }
    __asm
    {
    mov eax, 01h
    xor ecx, ecx
    xor edx, edx
    cpuid
    mov s3, edx
    mov s4, ecx
    }

    static char buf[100];
    sprintf_s(buf, "%08X%08X%08X%08X", s1, s2, s3, s4);
    cpuInfo = buf;
    return cpuInfo;
}
