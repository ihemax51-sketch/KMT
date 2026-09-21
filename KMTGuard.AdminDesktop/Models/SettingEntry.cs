namespace KMTGuard.AdminDesktop.Models;

public enum SettingStore
{
    Filter,
    GameServer
}

public sealed class SettingEntry
{
    public SettingStore Store { get; set; } = SettingStore.Filter;
    public string Category { get; set; } = "Unsorted";
    public int DisplayOrder { get; set; } = 999;
    public string SettingName { get; set; } = string.Empty;
    public string Value { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;

    public string ValueKind
    {
        get
        {
            if (Value.Equals("True", StringComparison.OrdinalIgnoreCase) ||
                Value.Equals("False", StringComparison.OrdinalIgnoreCase))
                return "Toggle";

            return int.TryParse(Value, out _) ? "Number" : "Text";
        }
    }
}
