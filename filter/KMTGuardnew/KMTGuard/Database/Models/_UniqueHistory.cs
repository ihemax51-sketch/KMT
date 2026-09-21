using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace KMTGuard.Database.Models
{
    public class _UniqueHistory
    {
        public int UniqueID { get; set; }
        public string KillerName { get; set; } = string.Empty;
        public byte State { get; set; }
        public long Time { get; set; }
        public int KilledRegionID { get; set; }
        public float KilledX { get; set; }
        public float KilledY { get; set; }
        public float KilledZ { get; set; }
        public int WorldID { get; set; }
        public byte MapType { get; set; }
        public int MapIndex { get; set; }
        public ConcurrentDictionary<string, int> DMGMETER { get; set; } = new();
    }
}
