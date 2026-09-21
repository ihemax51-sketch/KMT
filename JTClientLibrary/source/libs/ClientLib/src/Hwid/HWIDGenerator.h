#pragma once
#include <string>
#include <vector>

class HWIDGenerator {
public:
    std::string GenerateHWID();
    std::string BuildChallengeResponse(const std::string& challenge);
    static std::string GetSessionNonce();
    bool IsVirtualMachine();
private:
    static std::string GetBIOSInfo();
    static std::string GetDiskInfo();
    static std::string GetCPUInfo();
    static std::string Sha256Hex(const std::string& value);
    static std::string Base64Encode(const std::vector<unsigned char>& value);
    static bool SignChallenge(const std::string& canonical,
                              std::vector<unsigned char>& publicKey,
                              std::vector<unsigned char>& signature);
};
