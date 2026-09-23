#include "InternalPacketAuth.h"

#include <algorithm>
#include <ctime>
#include <map>
#include <vector>

namespace
{
    typedef unsigned long UInt32;

    BYTE s_secret[32] = { 0 };
    bool s_initialized = false;
    CRITICAL_SECTION s_authLock;

    struct ReplayEntry
    {
        __int64 expiresAt;
        ReplayEntry() : expiresAt(0) {}
        explicit ReplayEntry(__int64 value) : expiresAt(value) {}
    };
    std::map<std::string, ReplayEntry> s_replayCache;

    struct AuthLockInitializer
    {
        AuthLockInitializer() { InitializeCriticalSection(&s_authLock); }
        ~AuthLockInitializer() { DeleteCriticalSection(&s_authLock); }
    } s_authLockInitializer;

    UInt32 RotateRight(UInt32 value, int bits)
    {
        return (value >> bits) | (value << (32 - bits));
    }

    void Sha256Transform(UInt32 state[8], const BYTE block[64])
    {
        static const UInt32 constants[64] =
        {
            0x428a2f98UL,0x71374491UL,0xb5c0fbcfUL,0xe9b5dba5UL,0x3956c25bUL,0x59f111f1UL,0x923f82a4UL,0xab1c5ed5UL,
            0xd807aa98UL,0x12835b01UL,0x243185beUL,0x550c7dc3UL,0x72be5d74UL,0x80deb1feUL,0x9bdc06a7UL,0xc19bf174UL,
            0xe49b69c1UL,0xefbe4786UL,0x0fc19dc6UL,0x240ca1ccUL,0x2de92c6fUL,0x4a7484aaUL,0x5cb0a9dcUL,0x76f988daUL,
            0x983e5152UL,0xa831c66dUL,0xb00327c8UL,0xbf597fc7UL,0xc6e00bf3UL,0xd5a79147UL,0x06ca6351UL,0x14292967UL,
            0x27b70a85UL,0x2e1b2138UL,0x4d2c6dfcUL,0x53380d13UL,0x650a7354UL,0x766a0abbUL,0x81c2c92eUL,0x92722c85UL,
            0xa2bfe8a1UL,0xa81a664bUL,0xc24b8b70UL,0xc76c51a3UL,0xd192e819UL,0xd6990624UL,0xf40e3585UL,0x106aa070UL,
            0x19a4c116UL,0x1e376c08UL,0x2748774cUL,0x34b0bcb5UL,0x391c0cb3UL,0x4ed8aa4aUL,0x5b9cca4fUL,0x682e6ff3UL,
            0x748f82eeUL,0x78a5636fUL,0x84c87814UL,0x8cc70208UL,0x90befffaUL,0xa4506cebUL,0xbef9a3f7UL,0xc67178f2UL
        };

        UInt32 words[64];
        for (int i = 0; i < 16; ++i)
            words[i] = (static_cast<UInt32>(block[i * 4]) << 24) |
                       (static_cast<UInt32>(block[i * 4 + 1]) << 16) |
                       (static_cast<UInt32>(block[i * 4 + 2]) << 8) |
                       static_cast<UInt32>(block[i * 4 + 3]);
        for (int i = 16; i < 64; ++i)
        {
            const UInt32 s0 = RotateRight(words[i - 15], 7) ^ RotateRight(words[i - 15], 18) ^ (words[i - 15] >> 3);
            const UInt32 s1 = RotateRight(words[i - 2], 17) ^ RotateRight(words[i - 2], 19) ^ (words[i - 2] >> 10);
            words[i] = words[i - 16] + s0 + words[i - 7] + s1;
        }

        UInt32 a = state[0], b = state[1], c = state[2], d = state[3];
        UInt32 e = state[4], f = state[5], g = state[6], h = state[7];
        for (int i = 0; i < 64; ++i)
        {
            const UInt32 sum1 = RotateRight(e, 6) ^ RotateRight(e, 11) ^ RotateRight(e, 25);
            const UInt32 choice = (e & f) ^ ((~e) & g);
            const UInt32 temp1 = h + sum1 + choice + constants[i] + words[i];
            const UInt32 sum0 = RotateRight(a, 2) ^ RotateRight(a, 13) ^ RotateRight(a, 22);
            const UInt32 majority = (a & b) ^ (a & c) ^ (b & c);
            const UInt32 temp2 = sum0 + majority;
            h = g; g = f; f = e; e = d + temp1;
            d = c; c = b; b = a; a = temp1 + temp2;
        }
        state[0] += a; state[1] += b; state[2] += c; state[3] += d;
        state[4] += e; state[5] += f; state[6] += g; state[7] += h;
    }

    void Sha256(const BYTE* input, size_t length, BYTE output[32])
    {
        UInt32 state[8] =
        {
            0x6a09e667UL,0xbb67ae85UL,0x3c6ef372UL,0xa54ff53aUL,
            0x510e527fUL,0x9b05688cUL,0x1f83d9abUL,0x5be0cd19UL
        };
        size_t offset = 0;
        while (length - offset >= 64)
        {
            Sha256Transform(state, input + offset);
            offset += 64;
        }

        BYTE finalBlocks[128] = { 0 };
        const size_t remaining = length - offset;
        if (remaining != 0) memcpy(finalBlocks, input + offset, remaining);
        finalBlocks[remaining] = 0x80;
        const size_t finalLength = remaining < 56 ? 64 : 128;
        const unsigned __int64 bitLength = static_cast<unsigned __int64>(length) * 8;
        for (int i = 0; i < 8; ++i)
            finalBlocks[finalLength - 1 - i] = static_cast<BYTE>(bitLength >> (i * 8));
        Sha256Transform(state, finalBlocks);
        if (finalLength == 128) Sha256Transform(state, finalBlocks + 64);

        for (int i = 0; i < 8; ++i)
        {
            output[i * 4] = static_cast<BYTE>(state[i] >> 24);
            output[i * 4 + 1] = static_cast<BYTE>(state[i] >> 16);
            output[i * 4 + 2] = static_cast<BYTE>(state[i] >> 8);
            output[i * 4 + 3] = static_cast<BYTE>(state[i]);
        }
    }

    void HmacSha256(const BYTE key[32], const BYTE* data, size_t length, BYTE output[32])
    {
        BYTE innerPad[64], outerPad[64];
        for (size_t i = 0; i < 64; ++i)
        {
            const BYTE keyByte = i < 32 ? key[i] : 0;
            innerPad[i] = keyByte ^ 0x36;
            outerPad[i] = keyByte ^ 0x5c;
        }
        std::vector<BYTE> inner(innerPad, innerPad + 64);
        inner.insert(inner.end(), data, data + length);
        BYTE innerHash[32];
        Sha256(&inner[0], inner.size(), innerHash);
        BYTE outer[96];
        memcpy(outer, outerPad, 64);
        memcpy(outer + 64, innerHash, 32);
        Sha256(outer, sizeof(outer), output);
        SecureZeroMemory(innerHash, sizeof(innerHash));
        SecureZeroMemory(innerPad, sizeof(innerPad));
        SecureZeroMemory(outerPad, sizeof(outerPad));
        SecureZeroMemory(outer, sizeof(outer));
        if (!inner.empty()) SecureZeroMemory(&inner[0], inner.size());
    }

    int HexValue(char value)
    {
        if (value >= '0' && value <= '9') return value - '0';
        if (value >= 'A' && value <= 'F') return value - 'A' + 10;
        if (value >= 'a' && value <= 'f') return value - 'a' + 10;
        return -1;
    }

    bool DecodeHex(const std::string& text, BYTE* output, size_t outputLength)
    {
        if (text.size() != outputLength * 2) return false;
        for (size_t i = 0; i < outputLength; ++i)
        {
            const int high = HexValue(text[i * 2]);
            const int low = HexValue(text[i * 2 + 1]);
            if (high < 0 || low < 0) return false;
            output[i] = static_cast<BYTE>((high << 4) | low);
        }
        return true;
    }

    bool ConstantTimeEquals(const BYTE left[32], const BYTE right[32])
    {
        BYTE difference = 0;
        for (size_t i = 0; i < 32; ++i) difference |= left[i] ^ right[i];
        return difference == 0;
    }

    void AppendLittleEndian(std::vector<BYTE>& bytes, unsigned __int64 value, size_t count)
    {
        for (size_t i = 0; i < count; ++i)
            bytes.push_back(static_cast<BYTE>(value >> (i * 8)));
    }
}

bool InternalPacketAuth::Initialize(const std::string& sharedSecretHex)
{
    EnterCriticalSection(&s_authLock);
    SecureZeroMemory(s_secret, sizeof(s_secret));
    s_replayCache.clear();
    s_initialized = DecodeHex(sharedSecretHex, s_secret, sizeof(s_secret));
    if (!s_initialized)
        SecureZeroMemory(s_secret, sizeof(s_secret));
    LeaveCriticalSection(&s_authLock);
    return s_initialized;
}

void InternalPacketAuth::Shutdown()
{
    EnterCriticalSection(&s_authLock);
    SecureZeroMemory(s_secret, sizeof(s_secret));
    s_replayCache.clear();
    s_initialized = false;
    LeaveCriticalSection(&s_authLock);
}

InternalPacketAuth::ValidationResult InternalPacketAuth::ValidateRegistration(
    DWORD expectedGameId, BYTE version, DWORD gameId, __int64 issuedAtUnixSeconds,
    const BYTE nonce[16], const BYTE sessionKey[32], const BYTE mac[32])
{
    EnterCriticalSection(&s_authLock);
    const bool initialized = s_initialized;
    LeaveCriticalSection(&s_authLock);
    if (!initialized) return AUTH_NOT_INITIALIZED;
    if (version != 2) return AUTH_BAD_VERSION;
    if (gameId == 0 || gameId != expectedGameId) return AUTH_GAME_ID_MISMATCH;

    const __int64 now = _time64(NULL);
    const __int64 difference = now >= issuedAtUnixSeconds
        ? now - issuedAtUnixSeconds : issuedAtUnixSeconds - now;
    if (difference > 30) return AUTH_EXPIRED;

    BYTE expectedMac[32];
    if (!ComputeRegistrationMac(
            version, gameId, issuedAtUnixSeconds, nonce, sessionKey, expectedMac))
        return AUTH_NOT_INITIALIZED;
    const bool validMac = ConstantTimeEquals(expectedMac, mac);
    SecureZeroMemory(expectedMac, sizeof(expectedMac));
    if (!validMac)
        return AUTH_BAD_MAC;

    EnterCriticalSection(&s_authLock);
    const std::string replayKey(reinterpret_cast<const char*>(mac), 32);
    for (std::map<std::string, ReplayEntry>::iterator entry = s_replayCache.begin();
         entry != s_replayCache.end();)
    {
        if (entry->second.expiresAt < now)
            s_replayCache.erase(entry++);
        else
            ++entry;
    }
    if (s_replayCache.find(replayKey) != s_replayCache.end())
    {
        LeaveCriticalSection(&s_authLock);
        return AUTH_REPLAY;
    }
    if (s_replayCache.size() >= 8192) s_replayCache.erase(s_replayCache.begin());
    s_replayCache[replayKey] = ReplayEntry(now + 120);
    LeaveCriticalSection(&s_authLock);
    return AUTH_VALID;
}

bool InternalPacketAuth::ComputeRegistrationMac(
    BYTE version, DWORD gameId, __int64 issuedAtUnixSeconds,
    const BYTE nonce[16], const BYTE sessionKey[32], BYTE mac[32])
{
    if (nonce == NULL || sessionKey == NULL || mac == NULL)
        return false;

    std::vector<BYTE> signedBytes;
    signedBytes.reserve(61);
    signedBytes.push_back(version);
    AppendLittleEndian(signedBytes, gameId, 4);
    AppendLittleEndian(signedBytes, static_cast<unsigned __int64>(issuedAtUnixSeconds), 8);
    signedBytes.insert(signedBytes.end(), nonce, nonce + 16);
    signedBytes.insert(signedBytes.end(), sessionKey, sessionKey + 32);

    EnterCriticalSection(&s_authLock);
    const bool initialized = s_initialized;
    if (initialized) HmacSha256(s_secret, &signedBytes[0], signedBytes.size(), mac);
    LeaveCriticalSection(&s_authLock);
    SecureZeroMemory(&signedBytes[0], signedBytes.size());
    return initialized;
}

std::string InternalPacketAuth::ToHex(const BYTE* value, size_t length)
{
    static const char digits[] = "0123456789ABCDEF";
    std::string result(length * 2, '0');
    for (size_t i = 0; i < length; ++i)
    {
        result[i * 2] = digits[value[i] >> 4];
        result[i * 2 + 1] = digits[value[i] & 0x0F];
    }
    return result;
}
