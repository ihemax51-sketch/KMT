using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace KMTGuard.Database.Models
{
    public class _RefAchievementCondition
    {
        public int ID {get ; set; }
        public string Name {get ; set; } = string.Empty;
        public int RefAchievementID {get ; set; }
        public Int64 CompleteCount {get ; set; }
        public byte Type { get; set; }
    }
}
