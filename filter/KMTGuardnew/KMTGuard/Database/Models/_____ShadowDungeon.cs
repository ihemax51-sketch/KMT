using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace KMTGuard.Database.Models
{
    public class _____ShadowDungeon
    {
        public int ID { get; set; }
        public int CharID { get; set; }
        public int WorldID { get; set; }
        public int LayerID { get; set; }
        public bool IsActivated { get; set; }
        public bool TGCalled { get; set; }
        public bool CerberusCalled { get; set; }
        public bool IvyCalled { get; set; }
        public bool UruchiCalled { get; set; }
        public bool IsyCalled { get; set; }
    }
}
