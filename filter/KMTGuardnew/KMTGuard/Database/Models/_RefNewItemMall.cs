using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace KMTGuard.Database.Models
{
    public class _RefNewItemMall
    {
        public int ID { get; set; }
        public bool Service { get; set; }
        public string CategoryName { get; set; } = string.Empty;
        public byte Type { get; set; }
        public string CodeName128 { get; set; } = string.Empty;
        public int ItemID { get; set; }
        public int ItemCount { get; set; }
        public int Silk { get; set; }
        public bool ShowInNewBest { get; set; }
        public int ItemIndex { get; set; }
        public int ShowInNewBestIndex { get; set; }
    }
}
