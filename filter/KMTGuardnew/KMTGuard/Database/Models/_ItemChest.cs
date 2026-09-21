using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace KMTGuard.Database.Models
{
    public class _ItemChest
    {
        public int ID { get; set; }
        public int CharID { get; set; }
        public string ItemCodeName { get; set; } = string.Empty;
        public int ItemID {get ; set ; }
        public int Quantity {get ; set ; }
        public string Date {get ; set ; } = "";
        public string Type { get; set; } = "";
        public byte Plus {get ; set ; }
    }
}
