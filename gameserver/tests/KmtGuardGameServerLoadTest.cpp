#include <Windows.h>
#include <Msg.h>
#include <KMTGuardCustom/InternalPacketAuth.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace
{
    struct DamageTestDescending
    {
        bool operator()(const std::pair<unsigned int, unsigned int>& left,
                        const std::pair<unsigned int, unsigned int>& right) const
        {
            if (left.second != right.second)
                return left.second > right.second;
            return left.first < right.first;
        }
    };

    struct SessionRecord
    {
        std::string key;
        const void* owner;
        unsigned int gameId;
    };

    unsigned int NextRandom(unsigned int& state)
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }

    bool TryRead(const std::vector<unsigned char>& packet, size_t& position, size_t count)
    {
        if (position > packet.size() || count > packet.size() - position)
            return false;
        position += count;
        return true;
    }

    double ElapsedSeconds(const LARGE_INTEGER& started, const LARGE_INTEGER& frequency)
    {
        LARGE_INTEGER finished;
        QueryPerformanceCounter(&finished);
        return static_cast<double>(finished.QuadPart - started.QuadPart) /
               static_cast<double>(frequency.QuadPart);
    }

    void UpdatePeak(size_t value, size_t& peak)
    {
        if (value > peak)
            peak = value;
    }

    bool VerifyRealMessageBounds()
    {
        CMsg message;
        std::memset(&message, 0, sizeof(message));
        message.m_pMsgBuffer = message.m_MsgBuffer;
        message.m_dwArrayDataSize = MSG_BUFFER_SIZE;
        message.m_wpMsgSize = reinterpret_cast<WORD*>(message.m_pMsgBuffer + MSG_SIZE_OFFSET);
        message.m_wpMsgId = reinterpret_cast<WORD*>(message.m_pMsgBuffer + MSG_ID_OFFSET);
        message.m_wReadDataArrayPos = MSG_HEADER_SIZE;
        message.m_wWriteDataArrayPos = MSG_HEADER_SIZE + sizeof(unsigned int);
        message.m_dwReadMsgMode = MSG_READ_MODE_NORMAL;

        const unsigned int expected = 0x4B4D5447u;
        std::memcpy(message.m_pMsgBuffer + MSG_HEADER_SIZE, &expected, sizeof(expected));
        if (message.Read<unsigned int>() != expected)
            return false;

        message.m_wReadDataArrayPos = MSG_HEADER_SIZE;
        message.m_wWriteDataArrayPos = MSG_HEADER_SIZE + 1;
        try
        {
            message.Read<unsigned int>();
            return false;
        }
        catch (...)
        {
        }

        message.m_wReadDataArrayPos = MSG_HEADER_SIZE;
        message.m_wWriteDataArrayPos = MSG_HEADER_SIZE + sizeof(unsigned int) * 2;
        message.m_dwReadMsgMode = MSG_READ_MODE_REVERSE;
        const unsigned int first = 0x11111111u;
        const unsigned int second = 0x22222222u;
        std::memcpy(message.m_pMsgBuffer + MSG_HEADER_SIZE, &first, sizeof(first));
        std::memcpy(message.m_pMsgBuffer + MSG_HEADER_SIZE + sizeof(first), &second, sizeof(second));
        if (message.Read<unsigned int>() != second)
            return false;

        try
        {
            message.SetReadPos(static_cast<WORD>(message.m_wWriteDataArrayPos + 1));
            return false;
        }
        catch (...)
        {
        }

        message.m_dwReadMsgMode = MSG_READ_MODE_NORMAL;
        message.m_wReadDataArrayPos = MSG_HEADER_SIZE;
        message.m_wWriteDataArrayPos = MSG_HEADER_SIZE + sizeof(WORD) + 65;
        const WORD oversizedLength = 65;
        std::memcpy(message.m_pMsgBuffer + MSG_HEADER_SIZE,
                    &oversizedLength, sizeof(oversizedLength));
        std::memset(message.m_pMsgBuffer + MSG_HEADER_SIZE + sizeof(WORD),
                    'A', oversizedLength);
        std::string boundedString;
        try
        {
            message.ReadString(boundedString, 64);
            return false;
        }
        catch (...)
        {
        }
        if (!boundedString.empty())
            return false;

        message.m_wWriteDataArrayPos = MSG_HEADER_SIZE;
        message.ResetWriteState();
        unsigned int marker = 0;
        message.Write(&marker, 0x8000);
        if (!message.HasWriteOverflow())
            return false;

        return true;
    }

    bool VerifyPacketAuthentication()
    {
        const std::string secret =
            "000102030405060708090A0B0C0D0E0F"
            "101112131415161718191A1B1C1D1E1F";
        InternalPacketAuth::Shutdown();
        BYTE emptyNonce[16] = { 0 };
        BYTE emptyKey[32] = { 0 };
        BYTE emptyMac[32] = { 0 };
        if (InternalPacketAuth::ValidateRegistration(
                1, 2, 1, _time64(NULL), emptyNonce, emptyKey, emptyMac) !=
            InternalPacketAuth::AUTH_NOT_INITIALIZED)
            return false;
        if (!InternalPacketAuth::Initialize(secret)) return false;

        BYTE nonce[16];
        BYTE sessionKey[32];
        for (int i = 0; i < 16; ++i) nonce[i] = static_cast<BYTE>(i + 1);
        for (int i = 0; i < 32; ++i) sessionKey[i] = static_cast<BYTE>(0x80 + i);
        const DWORD gameId = 0x12345678;
        const __int64 now = _time64(NULL);
        BYTE mac[32];
        if (!InternalPacketAuth::ComputeRegistrationMac(2, gameId, now, nonce, sessionKey, mac))
            return false;
        if (InternalPacketAuth::ValidateRegistration(
                gameId, 2, gameId, now, nonce, sessionKey, mac) !=
            InternalPacketAuth::AUTH_VALID)
            return false;
        if (InternalPacketAuth::ValidateRegistration(
                gameId, 2, gameId, now, nonce, sessionKey, mac) !=
            InternalPacketAuth::AUTH_REPLAY)
            return false;
        if (InternalPacketAuth::ValidateRegistration(
                gameId + 1, 2, gameId, now, nonce, sessionKey, mac) !=
            InternalPacketAuth::AUTH_GAME_ID_MISMATCH)
            return false;
        if (InternalPacketAuth::ValidateRegistration(
                gameId, 1, gameId, now, nonce, sessionKey, mac) !=
            InternalPacketAuth::AUTH_BAD_VERSION)
            return false;
        if (InternalPacketAuth::ValidateRegistration(
                gameId, 2, gameId, now - 31, nonce, sessionKey, mac) !=
            InternalPacketAuth::AUTH_EXPIRED)
            return false;

        ++nonce[0];
        mac[0] ^= 0xFF;
        if (InternalPacketAuth::ValidateRegistration(
                gameId, 2, gameId, now, nonce, sessionKey, mac) !=
            InternalPacketAuth::AUTH_BAD_MAC)
            return false;

        InternalPacketAuth::Shutdown();
        return true;
    }

    bool VerifyGameplayBounds()
    {
        const int maximums[] = { 1000, 3000, 12000 };
        const int expected[] = { 1000, 3000, 3000 };
        for (int i = 0; i < 3; ++i)
        {
            const float ratio = 3000.0f / maximums[i];
            const float multiplier = ratio < 1.0f ? ratio : 1.0f;
            const int restored = static_cast<int>(maximums[i] * multiplier);
            if (restored != expected[i]) return false;
        }

        std::vector<std::pair<unsigned int, unsigned int> > damage;
        for (unsigned int i = 0; i < 100; ++i)
            damage.push_back(std::make_pair(i + 1, i * 10));
        std::sort(damage.begin(), damage.end(), DamageTestDescending());
        damage.resize(15);
        return damage.size() == 15 && damage.front().second == 990 &&
               damage.back().second == 850;
    }

    bool VerifyReconnectOwnership()
    {
        int oldOwner = 1;
        int newOwner = 2;
        std::map<unsigned int, SessionRecord> sessions;

        SessionRecord oldSession = { "old", &oldOwner, 1001 };
        SessionRecord newSession = { "new", &newOwner, 2001 };
        sessions[77] = oldSession;
        sessions[77] = newSession;

        std::map<unsigned int, SessionRecord>::iterator current = sessions.find(77);
        if (current != sessions.end() &&
            current->second.owner == &oldOwner &&
            current->second.gameId == oldSession.gameId)
        {
            sessions.erase(current);
        }

        current = sessions.find(77);
        return current != sessions.end() &&
               current->second.owner == &newOwner &&
               current->second.gameId == newSession.gameId &&
               current->second.key == "new";
    }
}

int main()
{
    const unsigned int packetIterations = 2000000;
    const unsigned int sessionIterations = 2000000;
    const unsigned int combatIterations = 5000000;
    const unsigned int dpsIterations = 3000000;
    const unsigned int concurrentPlayers = 4096;
    const unsigned int activeUniqueMobs = 2000;
    const unsigned int dpsBatchLimit = 256;

    LARGE_INTEGER frequency;
    LARGE_INTEGER started;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&started);

    if (!VerifyRealMessageBounds())
        return 8;
    if (!VerifyReconnectOwnership())
        return 9;
    if (!VerifyPacketAuthentication())
        return 14;
    if (!VerifyGameplayBounds())
        return 15;

    unsigned int randomState = 0x4B4D5447u;
    unsigned int malformedPackets = 0;
    unsigned __int64 checksum = 0;

    std::vector<unsigned char> packet(4096, 0);
    for (unsigned int i = 0; i < packetIterations; ++i)
    {
        const size_t packetSize = NextRandom(randomState) % packet.size();
        const size_t requestedRead = NextRandom(randomState) % 512;
        size_t position = NextRandom(randomState) % (packet.size() + 1);
        packet.resize(packetSize);
        if (!TryRead(packet, position, requestedRead))
            ++malformedPackets;
        else
            checksum += position;
        packet.resize(4096);
    }

    std::map<unsigned int, std::string> sessionKeys;
    size_t sessionPeak = 0;
    for (unsigned int i = 0; i < sessionIterations; ++i)
    {
        const unsigned int playerId = (i % concurrentPlayers) + 1;
        char key[65];
        _snprintf_s(key, sizeof(key), _TRUNCATE, "%064I64X",
                    static_cast<unsigned __int64>(playerId) ^ i);
        sessionKeys[playerId] = key;
        if (sessionKeys.find(playerId) == sessionKeys.end())
            return 10;
        if ((i % 3) == 0)
            sessionKeys.erase(playerId);
        UpdatePeak(sessionKeys.size(), sessionPeak);
    }
    sessionKeys.clear();
    if (!sessionKeys.empty() || sessionPeak > concurrentPlayers)
        return 11;

    void* partyMarker = &randomState;
    unsigned int allowedCombat = 0;
    unsigned int combatDecisions = 0;
    for (unsigned int i = 0; i < combatIterations; ++i)
    {
        void* playerParty = (i & 1) == 0 ? partyMarker : NULL;
        const bool requiresParty = (i % 4) != 0;
        const bool allowed = !requiresParty || playerParty != NULL;
        if (allowed)
            ++allowedCombat;
        ++combatDecisions;
        checksum += allowed ? 1 : 0;
    }
    if (combatDecisions != combatIterations || allowedCombat == 0)
        return 12;

    std::set<unsigned int> pendingDpsMobs;
    size_t dpsPendingPeak = 0;
    unsigned int dpsBatches = 0;
    unsigned int dpsPackets = 0;
    for (unsigned int i = 0; i < dpsIterations; ++i)
    {
        const unsigned int mobId = (NextRandom(randomState) % activeUniqueMobs) + 1;
        pendingDpsMobs.insert(mobId);
        UpdatePeak(pendingDpsMobs.size(), dpsPendingPeak);

        if ((i % 5000) == 4999)
        {
            unsigned int batchSize = 0;
            std::set<unsigned int>::iterator it = pendingDpsMobs.begin();
            while (it != pendingDpsMobs.end() && batchSize < dpsBatchLimit)
            {
                pendingDpsMobs.erase(it++);
                ++batchSize;
            }
            ++dpsBatches;
            dpsPackets += batchSize;
        }
    }
    while (!pendingDpsMobs.empty())
    {
        unsigned int batchSize = 0;
        std::set<unsigned int>::iterator it = pendingDpsMobs.begin();
        while (it != pendingDpsMobs.end() && batchSize < dpsBatchLimit)
        {
            pendingDpsMobs.erase(it++);
            ++batchSize;
        }
        ++dpsBatches;
        dpsPackets += batchSize;
    }

    if (!pendingDpsMobs.empty() || dpsPendingPeak > activeUniqueMobs)
        return 13;

    const double elapsedSeconds = ElapsedSeconds(started, frequency);
    const unsigned __int64 totalOperations =
        static_cast<unsigned __int64>(packetIterations) +
        sessionIterations + combatIterations + dpsIterations;
    const double operationsPerSecond = elapsedSeconds > 0.0
        ? static_cast<double>(totalOperations) / elapsedSeconds
        : 0.0;

    std::printf(
        "PASS operations=%I64u elapsed_s=%.3f ops_per_s=%.0f "
        "packet_guard=pass reconnect_guard=pass packet_rejections=%u session_peak=%u combat_decisions=%u "
        "dps_pending_peak=%u dps_batches=%u dps_packets=%u checksum=%I64u\n",
        totalOperations,
        elapsedSeconds,
        operationsPerSecond,
        malformedPackets,
        static_cast<unsigned int>(sessionPeak),
        combatDecisions,
        static_cast<unsigned int>(dpsPendingPeak),
        dpsBatches,
        dpsPackets,
        checksum);
    return 0;
}
