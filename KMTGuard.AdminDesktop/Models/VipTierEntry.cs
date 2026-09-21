namespace KMTGuard.AdminDesktop.Models;

public sealed class VipTierEntry
{
    public int RankCode { get; set; }
    public string DisplayName { get; set; } = string.Empty;
    public int MinSilk { get; set; }
    public int IconID { get; set; }
    public string IconPath { get; set; } = string.Empty;
    public string BuffSkillCode { get; set; } = string.Empty;
    public int AssignedPlayers { get; set; }
}
