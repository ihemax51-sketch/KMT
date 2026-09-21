using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace KMTGuard.Database.Models
{
    public class _RefAttendanceReward
    {
        public int ID { get; set; }
        public int ItemID { get ; set; }
        public string ItemCodeName128 { get ; set; } = string.Empty;
        public int ItemCount { get ; set; }
        public int DayCount { get ; set; }
    }
}
