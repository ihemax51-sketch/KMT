#region

using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Text.Json.Serialization;
using KMTGuard.Database.Models;
using KMTGuard.Helpers;
using SilkroadSecurityAPI;

#endregion

namespace KMTGuard.SessionManager;

public class SessionData : ISessionData
{// Add: INT/STR من CharacterSelection
    public ushort StatSTR { get; set; }
    public ushort StatINT { get; set; }
    public int BaseStr { get; set; }
    public int BaseInt { get; set; }
    public SessionData()
    {
        State = new State();
        PlayerTitles = new();
        PlayerTitleColors = new();
        PlayerIcons = new();
        CharacterChest = new();
        PendingChestClaims = new();
        CharacterAchievement = new();
        CharacterAchievementCondition = new();
        CharacterNewReverseSavedLocations = new();
        PetDataDictionary = new();
        CharPetList = new();
        GatewayCredential = new();
    }
    public IState State { get; }
    public int JID { get; set; }
    public int Charid { get; set; }
    public string Charname { get; set; } = string.Empty;
    public string MailAddress { get; set; } = string.Empty;
    public uint UniqueCharId { get; set; }
    public string Hwid { get; set; } = string.Empty;
    public bool FirstSpawn { get; set; } = true;
    public DateTime CharacterReadyAtUtc { get; set; } = DateTime.UtcNow;
    public bool OfflineStall { get; set; }
    public DateTime? OfflineStallActivatedAtUtc { get; set; }
    public DateTime? OfflineStallDetachedAtUtc { get; set; }
    public ConcurrentList<int> PlayerTitles { get; set; }
    public ConcurrentDictionary<int, PlayerTitleColor> PlayerTitleColors { get; set; }
    public ConcurrentDictionary<int, PlayerIcon> PlayerIcons { get; set; }
    public ConcurrentDictionary<int, _ItemChest> CharacterChest { get; set; }
    public ConcurrentDictionary<int, Guid> PendingChestClaims { get; set; }
    public ConcurrentDictionary<int, _Achievement> CharacterAchievement { get; set; }
    public ConcurrentDictionary<int, _AchievementCondition> CharacterAchievementCondition { get; set; }
    public DateTime LastAchievementTitleActionUtc { get; set; } = DateTime.MinValue;
    public ConcurrentDictionary<int, _NewReverseSavedLocations> CharacterNewReverseSavedLocations { get; set; }
    public ConcurrentList<uint> CharPetList { get; set; }

    public ConcurrentDictionary<uint, PetInfo> PetDataDictionary { get; set; }

    public int AttendanceDayCount { get; set; }
    public DateTime LastAttendanceStateRequestUtc { get; set; } = DateTime.MinValue;
    public DateTime LastAttendanceRecordRequestUtc { get; set; } = DateTime.MinValue;
    public DateTime LastAttendanceClaimRequestUtc { get; set; } = DateTime.MinValue;

    public uint SELECTEDUNIQUEID { get; set; } = 0;
    public PendingTradeSellCaptcha? PendingTradeSellCaptcha { get; set; }

    public bool isSilkStall { get; set; }
    public uint lastStallUId { get; set; }
    public bool TitleManagerPacket { get; set; } = false;
    public bool IconMgrPacket { get; set; } = false;
    public bool UniqueHistoryPacket { get; set; } = false;
    public bool EventRegisterPacket { get; set; } = false;
    public bool EventSchedulePacket { get; set; } = false;
    public bool ChestPacket { get; set; } = false;

    public bool RankCategoryPacket { get; set; } = false;
    public uint CharObjID { get; set; }
    public byte CurLevel { get; set; }
    public byte HwanLevel { get; set; }
    public bool EUChar { get; set; }
    public bool CHChar { get; set; }
    public bool MaleChar { get; set; }
    public bool FemaleChar { get; set; }
    public byte JobType { get; set; }
    public int LatestRegion {  get; set; }
    public int WorldID { get; set; }
    public int WorldLayerID { get; set; }
    public string JobName { get; set; } = string.Empty;

    public bool HideCharInformation { get; set; } = false;
    public bool SecondPwRememberPC { get; set; }
    public uint FellowPetUniqueID { get; set; }
    public Int64 FellowPetID64 { get; set; }
    public int FellowItemID { get; set; }
    public bool IsFellowSummoned { get; set; }
    public uint FellowRuntimeObjectID { get; set; }
    public uint FellowRefObjID { get; set; }

    public void ClearFellowRuntimeState()
    {
        IsFellowSummoned = false;
        FellowRuntimeObjectID = 0;
        FellowRefObjID = 0;
    }

    public DateTime LAST_REVERSE_TIME { get; set; } = new DateTime(2020, 12, 31);
    public DateTime LAST_LOCK_MAIL_TIME { get; set; } = new DateTime(2020, 12, 31);
    public DateTime LAST_UNLOCK_MAIL_TIME { get; set; } = new DateTime(2020, 12, 31);
    public DateTime LAST_STALL_TIME { get; set; } = new DateTime(2020, 12, 31);
    public DateTime LAST_EXCHANGE_TIME { get; set; } = new DateTime(2020, 12, 31);

    public DateTime LAST_GUILD_INVITE_TIME { get; set; } = new DateTime(2020, 12, 31);

    public DateTime LAST_UNION_INVITE_TIME { get; set; } = new DateTime(2020, 12, 31);
    public DateTime LAST_LIVE_ITEM_DELAY { get; set; } = new DateTime(2020, 12, 31);
    public DateTime LAST_GLOBAL_TIME { get; set; } = new DateTime(2020, 12, 31);
    public DateTime LAST_SPAWN_TIME { get; set; } = new DateTime(2020, 12, 31);
    public DateTime LAST_RESTART_TIME { get; set; } = new DateTime(2020, 12, 31);
    public DateTime LAST_EXIT_TIME { get; set; } = new DateTime(2020, 12, 31);
    public DateTime LAST_ZERK_TIME { get; set; } = new DateTime(2020, 12, 31);

    public DateTime LAST_CHAR_INFO_DELAY { get; set; } = new DateTime(2020, 12, 31);
    public DateTime LastRegionMovementUtc { get; set; } = DateTime.UtcNow;
    public DateTime LastAutoPvpCheckUtc { get; set; } = DateTime.MinValue;
    public int EventSuitTeamCharId { get; set; }
    public string EventSuitTeamEventName { get; set; } = string.Empty;
    public byte EventSuitTeam { get; set; }
    public DateTime EventSuitTeamFetchedUtc { get; set; } = DateTime.MinValue;
    public DateTime LastRegionReturnUtc { get; set; } = new DateTime(2020, 12, 31);
    public byte PendingRegionTravelMethod { get; set; }
    public DateTime PendingRegionTravelUtc { get; set; } = DateTime.MinValue;


    public int LockCode { get; set; }
    public int UnLockCode { get; set; }
    // GATEWAY

    public long? LastSnowshieldUsage { get; set; }
    public long? LastSkillUsage { get; set; }
    public long? LastBuffUsage { get; set; }

    public bool OnTransport { get; set; }
    public uint TransportUniqueId { get; set; }
    public int SecondaryPassword { get; set; }
    public byte locale { get; set; }
    public string user_id { get; set; } = string.Empty;
    public GatewayCredential GatewayCredential { get; }
    public ushort ServerID { get; set; }



   public bool IsInParty { get; set; }
   
   public bool IsPartyMaster { get; set; }
}
