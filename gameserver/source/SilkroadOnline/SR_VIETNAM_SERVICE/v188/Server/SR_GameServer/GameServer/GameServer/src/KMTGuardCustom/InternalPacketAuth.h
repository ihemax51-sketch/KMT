#pragma once

#include <Windows.h>
#include <string>

namespace InternalPacketAuth
{
    enum ValidationResult
    {
        AUTH_VALID = 0,
        AUTH_NOT_INITIALIZED,
        AUTH_BAD_VERSION,
        AUTH_GAME_ID_MISMATCH,
        AUTH_EXPIRED,
        AUTH_REPLAY,
        AUTH_BAD_MAC
    };

    bool Initialize(const std::string& sharedSecretHex);
    void Shutdown();
    ValidationResult ValidateRegistration(
        DWORD expectedGameId,
        BYTE version,
        DWORD gameId,
        __int64 issuedAtUnixSeconds,
        const BYTE nonce[16],
        const BYTE sessionKey[32],
        const BYTE mac[32]);
    bool ComputeRegistrationMac(
        BYTE version,
        DWORD gameId,
        __int64 issuedAtUnixSeconds,
        const BYTE nonce[16],
        const BYTE sessionKey[32],
        BYTE mac[32]);
    std::string ToHex(const BYTE* value, size_t length);
}
