namespace KMTGuard.Clientless;

public sealed class ClientlessHuntPolicy
{
    public bool Enabled { get; init; }
    public bool AttackNormal { get; init; } = true;
    public bool AttackUnique { get; init; } = true;
    public bool UniquePriority { get; init; } = true;
    public bool UseSkills { get; init; } = true;
    public bool UseBasicAttack { get; init; } = true;
    public byte HpPotionPercent { get; init; } = 60;
    public byte MpPotionPercent { get; init; } = 40;
    public short StuckSeconds { get; init; } = 15;
    public short TargetTimeoutSeconds { get; init; } = 30;
}

public sealed class ClientlessHuntArea
{
    public int ID { get; init; }
    public string City { get; init; } = string.Empty;
    public byte SlotNumber { get; init; }
    public string DisplayName { get; init; } = string.Empty;
    public int RegionID { get; init; }
    public float PosX { get; init; }
    public float PosY { get; init; }
    public float PosZ { get; init; }
    public float Radius { get; init; }
}

public sealed class ClientlessCombatSkill
{
    public uint ID { get; init; }
    public string CodeName128 { get; init; } = string.Empty;
    public int BasicLevel { get; init; }
    public int GroupID { get; init; }
    public int CooldownMs { get; init; }
    public int CastDurationMs { get; init; }
    public int ActionRange { get; init; }
    public int ConsumeMp { get; init; }
    public int ReqCastWeapon1 { get; init; }
    public int ReqCastWeapon2 { get; init; }
    public bool TargetsSelf { get; init; }
    public bool IsImbue { get; init; }
}

public sealed class ClientlessPotionSlot
{
    public byte Slot { get; init; }
    public byte TypeID4 { get; init; }
    public ushort TypeID { get; init; }
    public int CooldownMs { get; init; } = 15100;
}

public sealed class ClientlessSpeedSlot
{
    public byte Slot { get; init; }
    public ushort TypeID { get; init; }
    public string CodeName128 { get; init; } = string.Empty;
}

public sealed class ClientlessPetSlot
{
    public byte Slot { get; init; }
    public byte PetKind { get; init; }
    public ushort TypeID { get; init; }
    public string CodeName128 { get; init; } = string.Empty;
}

public sealed class ClientlessPetSupplySlot
{
    public byte Slot { get; init; }
    public byte SupplyKind { get; init; }
    public ushort TypeID { get; init; }
    public string CodeName128 { get; init; } = string.Empty;
}
