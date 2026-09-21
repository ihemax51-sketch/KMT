namespace KMTGuard.Features.Skills
{
    public sealed class SkillRule
    {
        public int SkillID { get; set; }
        public int? RegionID { get; set; } // NULL = قاعدة عامة
        public bool Blocked { get; set; }
        public int DelaySeconds { get; set; }
        public int? MinLevel { get; set; }
        public bool OnlyInJob { get; set; }
    }
}
