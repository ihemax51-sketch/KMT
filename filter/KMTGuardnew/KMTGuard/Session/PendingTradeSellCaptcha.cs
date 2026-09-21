namespace KMTGuard.SessionManager;

public sealed class PendingTradeSellCaptcha
{
    public int Code { get; set; }
    public DateTime ExpiresAtUtc { get; set; }
    public int Attempts { get; set; }
    public int PetUniqueId { get; set; }
    public byte PetSlot { get; set; }
    public short Count { get; set; }
    public int NpcUniqueId { get; set; }
}
