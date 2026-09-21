namespace KMTGuard.Database.Models
{
    public class _KillerAnimation
    {
        public int ID { get; set; }
        public bool Service { get; set; }
        public int SortOrder { get; set; }
        public string CodeName { get; set; } = string.Empty;
        public string DisplayName { get; set; } = string.Empty;
        public int AnimationID { get; set; }
        public int Price { get; set; }
        public byte PaymentType { get; set; }
    }
}
