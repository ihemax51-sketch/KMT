using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using KMTGuard.AsyncServerManager;
using KMTGuard.Helpers;
using SilkroadSecurityAPI;

namespace KMTGuard.SessionManager
{
    public interface ISession
    {
        IAsyncServer AsyncServer { get; init; }
        Packet? TempPacket { get; set; }
        Guid ClientGuid { get; set; }
        string ClientIp { get; set; }
        Task SendToClient(Packet packet);
        void QueueClientPacketAfterCurrentServerPacket(Packet packet);
        Task SendToServer(Packet packet);
        Task SendNotice(string message);
        Task Start();
        void Stop();
        void Stop(string reason);
        bool IsStopped { get; }
        bool ClientDetached { get; }
        Task ShutdownCompletion { get; }
        bool TryDetachClientTransport(string reason);
        #region Features
        string PlayerUserID { get; set; }
        bool CharacterGameReady { get; set; }
        bool IsManagedClientless { get; set; }
        bool IsSystemClientless { get; set; }
        bool IsExternalBot { get; set; }
        ISessionData SessionData { get; init; }
        #endregion

        #region Protection

        // Packet Modification
        int PacketLength { get; set; }
        // False Packets
        bool CharnameSent { get; set; }
        bool CharScreen { get; set; }
        bool UserLoggedIn { get; set; }
        DateTime LastPing { get; set; }
        string HwidChallenge { get; set; }
        string HwidChallengeRole { get; set; }
        long HwidChallengeIssuedAtUnix { get; set; }
        DateTime HwidChallengeExpiresAt { get; set; }
        string DeviceKeyThumbprint { get; set; }
        string DevicePublicKey { get; set; }
        string VerifiedHwidNonce { get; set; }
        bool QuickLoginNonceRefreshPending { get; set; }
        bool PendingQuickLogin { get; set; }
        bool PendingPrimaryLogin { get; set; }
        long PendingPrimaryLoginStartedAt { get; set; }
        GatewayAuthenticationState GatewayAuthenticationState { get; set; }
        int SecondaryPasswordFailures { get; set; }
        DateTime SecondaryPasswordBlockedUntil { get; set; }
        string GameServerPacketKey { get; }
        #endregion
    }
}
