using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using KMTGuard.Database.Models;
using KMTGuard.Helpers;
using static KMTGuard.Server.AgentPacketHandler.CustomGameServerPacketHandler;

namespace KMTGuard.SessionManager;
public interface ISessionData
{

    int BaseStr { get; set; }
    int BaseInt { get; set; }
    ushort StatSTR { get; set; }
    ushort StatINT { get; set; }
    IState State { get; }
    int JID { get; set; }
    int Charid { get; set; }
    string Charname { get; set; }
    string MailAddress { get; set; }
    uint UniqueCharId { get; set; }
    string Hwid { get; set; }
    bool FirstSpawn { get; set; }
    DateTime CharacterReadyAtUtc { get; set; }
    bool OfflineStall { get; set; }
    DateTime? OfflineStallActivatedAtUtc { get; set; }
    DateTime? OfflineStallDetachedAtUtc { get; set; }
    bool isSilkStall { get; set; }
    List<int> PlayerTitles { get; set; }
    Dictionary<int, PlayerTitleColor> PlayerTitleColors { get; set; }
    Dictionary<int, PlayerIcon> PlayerIcons { get; set; }
    ConcurrentDictionary<int, _ItemChest> CharacterChest { get; set; }
    ConcurrentDictionary<int, Guid> PendingChestClaims { get; set; }
    Dictionary<int, _Achievement> CharacterAchievement { get; set; }
    Dictionary<int, _AchievementCondition> CharacterAchievementCondition { get; set; }
    DateTime LastAchievementTitleActionUtc { get; set; }
    Dictionary<int, _NewReverseSavedLocations> CharacterNewReverseSavedLocations { get; set; }

    List<uint> CharPetList { get; set; }

    Dictionary<uint, PetInfo> PetDataDictionary { get; set; }

    int AttendanceDayCount { get; set; }
    DateTime LastAttendanceStateRequestUtc { get; set; }
    DateTime LastAttendanceRecordRequestUtc { get; set; }
    DateTime LastAttendanceClaimRequestUtc { get; set; }

    uint SELECTEDUNIQUEID { get; set; }
    PendingTradeSellCaptcha? PendingTradeSellCaptcha { get; set; }


    uint lastStallUId { get; set; }
    bool TitleManagerPacket { get; set; }
    bool IconMgrPacket { get; set; }
    bool UniqueHistoryPacket { get; set; }
    bool EventRegisterPacket { get; set; }

    bool EventSchedulePacket { get; set; }
    bool RankCategoryPacket { get; set; }
    uint CharObjID { get; set; }
    byte CurLevel { get; set; }

    byte HwanLevel { get; set; }
    bool EUChar { get; set; }
    bool CHChar { get; set; }
    bool MaleChar { get; set; }
    bool FemaleChar { get; set; }

    byte JobType { get; set; }
    int LatestRegion { get; set; }
    int WorldID { get; set; }
    int WorldLayerID { get; set; }
    string JobName { get; set; }

    bool HideCharInformation { get; set; }
    bool SecondPwRememberPC { get; set; }
    uint FellowPetUniqueID { get; set; }
    Int64 FellowPetID64 { get; set; }
    int FellowItemID { get; set; }
    bool IsFellowSummoned { get; set; }
    uint FellowRuntimeObjectID { get; set; }
    uint FellowRefObjID { get; set; }
    void ClearFellowRuntimeState();
    DateTime LAST_REVERSE_TIME { get; set; }
    DateTime LAST_LOCK_MAIL_TIME { get; set; }

    DateTime LAST_UNLOCK_MAIL_TIME { get; set; }
    DateTime LAST_STALL_TIME { get; set; }
    DateTime LAST_EXCHANGE_TIME { get; set; }
    DateTime LAST_ZERK_TIME { get; set; }
    DateTime LAST_GUILD_INVITE_TIME { get; set; }
    DateTime LAST_UNION_INVITE_TIME { get; set; }
    DateTime LAST_LIVE_ITEM_DELAY { get; set; }
    DateTime LAST_GLOBAL_TIME { get; set; }
    DateTime LAST_SPAWN_TIME { get; set; }
    DateTime LAST_RESTART_TIME { get; set; }
    DateTime LAST_EXIT_TIME { get; set; }
    DateTime LAST_CHAR_INFO_DELAY { get; set; }
    DateTime LastRegionMovementUtc { get; set; }
    DateTime LastAutoPvpCheckUtc { get; set; }
    int EventSuitTeamCharId { get; set; }
    string EventSuitTeamEventName { get; set; }
    byte EventSuitTeam { get; set; }
    DateTime EventSuitTeamFetchedUtc { get; set; }
    DateTime LastRegionReturnUtc { get; set; }
    byte PendingRegionTravelMethod { get; set; }
    DateTime PendingRegionTravelUtc { get; set; }

    long? LastBuffUsage { get; set; }
    long? LastSnowshieldUsage { get; set; }
    long? LastSkillUsage { get; set; }

    bool OnTransport { get; set; }
    uint TransportUniqueId { get; set; }


    int LockCode { get; set; }
    int UnLockCode { get; set; }
    int SecondaryPassword { get; set; }
    //// NEWS
    byte locale { get; set; }
    string user_id { get; set; }
    string user_pw { get; set; }
    ushort ServerID { get; set; }

    bool IsInParty { get; set; }

    bool IsPartyMaster { get; set; }
}
