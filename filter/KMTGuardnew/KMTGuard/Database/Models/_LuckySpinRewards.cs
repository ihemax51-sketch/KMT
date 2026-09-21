using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace KMTGuard.Database.Models
{
    public class _LuckySpinRewards
    {
        public int ID { get; set; }
        public int ItemID { get; set; }
        public int Amount { get; set; }
        public int Rate { get; set; }
    }
}
