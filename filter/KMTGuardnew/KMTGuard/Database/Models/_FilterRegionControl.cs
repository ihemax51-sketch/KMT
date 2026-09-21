namespace KMTGuard.Database.Models
{
    /// <summary>
    /// Clean Region Control contract. Admission choices are mutually exclusive
    /// modes, so contradictory combinations cannot be stored.
    /// </summary>
    public class _FilterRegionControl
    {
        public int ID { get; set; }
        public int WorldID { get; set; }
        public int RegionID { get; set; }
        public string? RuleName { get; set; }
        public bool Enabled { get; set; } = true;

        public byte BuildMode { get; set; } // 0 Any, 1 STR, 2 INT, 3 Hybrid
        public byte JobMode { get; set; } // 0 Any, 1 Jobless, 2 Any job, 3 Trader, 4 Thief, 5 Hunter
        public byte RaceMode { get; set; } // 0 Any, 1 Chinese, 2 European
        public byte PartyMode { get; set; } // 0 Any, 1 Party required, 2 Solo required
        public byte MinLevel { get; set; }
        public byte MaxLevel { get; set; }

        public bool AllowTeleport { get; set; } = true;
        public bool AllowReverse { get; set; } = true;
        public bool AllowTrace { get; set; } = true;
        public bool AllowMovement { get; set; } = true;
        public bool AllowChat { get; set; } = true;
        public bool AllowGlobalChat { get; set; } = true;
        public bool AllowParty { get; set; } = true;
        public bool AllowExchange { get; set; } = true;
        public bool AllowStall { get; set; } = true;
        public bool AllowPvP { get; set; } = true;
        public bool AllowAlchemy { get; set; } = true;
        public bool AllowSpecialItems { get; set; } = true;
        public bool AllowBerserk { get; set; } = true;
        public byte AutoPvpCape { get; set; }
        public int InactivityReturnSeconds { get; set; }

        public byte EventSuitMode { get; set; } // 0 Off, 1 FFA, 2 Teams
        public string? ManagedEventCode { get; set; }
        public DateTime? ManagedAtUtc { get; set; }

        // Read-only aliases keep existing action guards on this centralized row.
        public bool Allow_IntCharacter => BuildMode is 0 or 2;
        public bool Allow_StrCharacter => BuildMode is 0 or 1;
        public bool Allow_HybridCharacter => BuildMode is 0 or 3;
        public bool Allow_Jobless => JobMode is 0 or 1;
        public bool Allow_Trader => JobMode is 0 or 2 or 3;
        public bool Allow_Thief => JobMode is 0 or 2 or 4;
        public bool Allow_Hunter => JobMode is 0 or 2 or 5;
        public bool Allow_Chinese => RaceMode is 0 or 1;
        public bool Allow_European => RaceMode is 0 or 2;
        public byte EntryPartyMode => PartyMode;
        public bool Enable_TeleportEntry => AllowTeleport;
        public bool Enable_Reverse => AllowReverse;
        public bool Enable_Trace => AllowTrace;
        public bool Enable_Move => AllowMovement;
        public bool Enable_Chat => AllowChat;
        public bool Enable_Global => AllowGlobalChat;
        public bool Enable_Party => AllowParty;
        public bool Enable_Exchange => AllowExchange;
        public bool Enable_Stall => AllowStall;
        public bool Enable_PvP => AllowPvP;
        public bool Enable_Alchemy => AllowAlchemy;
        public bool Enable_AdvElixir => AllowAlchemy;
        public bool Enable_FellowScroll => AllowSpecialItems;
        public bool Enable_ResurrectionScroll => AllowSpecialItems;
        public bool Enable_Zerk => AllowBerserk;
        public byte Enable_AutoPvP => AutoPvpCape;
        public bool Enable_EventSuit => EventSuitMode != 0;
        public byte EventSuit_Team => EventSuitMode == 2 ? (byte)1 : (byte)0;
        public bool Enable_JobMode => JobMode != 1;
        public bool Enable_InactivityReturn => InactivityReturnSeconds > 0;
        public int InactivitySeconds => InactivityReturnSeconds;
    }
}
