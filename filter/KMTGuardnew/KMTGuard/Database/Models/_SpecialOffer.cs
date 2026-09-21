using System;

namespace KMTGuard.Database.Models
{
    public class _SpecialOffer
    {
        public int ID { get; set; }
        public bool Service { get; set; }
        public int SortOrder { get; set; }
        public string Title { get; set; } = string.Empty;
        public int ItemID { get; set; }
        public int ItemCount { get; set; }
        public string? CodeName128 { get; set; }
        public int MainPrice { get; set; }
        public int SalePrice { get; set; }
        public byte PaymentType { get; set; }
        public byte PreviewMode { get; set; }
        public int PreviewRefObjID { get; set; }
        public string? PreviewImagePath { get; set; }
        public DateTime? StartDate { get; set; }
        public DateTime? EndDate { get; set; }
    }
}
