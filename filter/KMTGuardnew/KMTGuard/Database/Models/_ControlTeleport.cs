namespace KMTGuard.Database.Models
{
    public class _ControlTeleport
    {
        public int ID { get; set; }
        public int RefTeleportID { get; set; }
        public bool OnlyIntPlayer { get; set; }
        public bool OnlyStrPlayer { get; set; }
        public bool EUOnly { get; set; }
        public bool CHOnly { get; set; }
        public bool OnlyJobMode { get; set; }
        public bool OnlyTrader { get; set; }
        public bool OnlyHunter { get; set; }
        public bool OnlyThief { get; set; }
        public bool OnlyOnParty { get; set; }
    }
}
