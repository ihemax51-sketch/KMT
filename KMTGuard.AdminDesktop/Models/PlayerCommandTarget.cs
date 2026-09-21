namespace KMTGuard.AdminDesktop.Models;

public sealed class PlayerCommandTarget
{
    public int CharId { get; init; }
    public int UserJid { get; init; }
    public string CharacterName { get; init; } = string.Empty;
    public int Level { get; init; }
    public int RegionId { get; init; }
    public long Gold { get; init; }
    public int SilkOwn { get; init; }
    public int SilkGift { get; init; }
    public int SilkPoint { get; init; }
}
