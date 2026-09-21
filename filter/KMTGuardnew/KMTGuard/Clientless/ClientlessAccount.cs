namespace KMTGuard.Clientless;

public sealed class ClientlessAccount
{
    public int ID { get; set; }
    public bool Enabled { get; set; }
    public byte Locale { get; set; } = 22;
    public ushort ShardID { get; set; } = 64;
    public string AccountName { get; set; } = string.Empty;
    public string AccountPassword { get; set; } = string.Empty;
    public string CharacterName { get; set; } = string.Empty;
    public string City { get; set; } = "Unassigned";
    public string AgentAuthMode { get; set; } = "Auto";
    public int AgentAuthDelayMs { get; set; } = 1500;
    public string AgentAuthPaddingHex { get; set; } = string.Empty;
    public int LaunchDelayMs { get; set; } = 1000;
    public int ReconnectDelaySeconds { get; set; } = 30;
    public string SystemRole { get; set; } = string.Empty;
    public bool HuntEnabled { get; set; } = true;
    public int? HomeRegionID { get; set; }
    public float? HomeX { get; set; }
    public float? HomeY { get; set; }
    public float? HomeZ { get; set; }
    public int TownParkingRegionID { get; set; }
    public float TownParkingX { get; set; }
    public float TownParkingY { get; set; }
    public float TownParkingZ { get; set; }
    public int? HuntAreaID { get; set; }
    public string HuntAreaName { get; set; } = string.Empty;
    public int HuntRegionID { get; set; }
    public float HuntX { get; set; }
    public float HuntY { get; set; }
    public float HuntZ { get; set; }
    public float HuntRadius { get; set; } = 50;
}
