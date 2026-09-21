using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace KMTGuard.Database.Models
{
    public class _bypassHwidbyIP
    {
        public int ID { get; set; }
        public string IP { get; set; } = string.Empty;
        public int Limit { get; set; }
    }
    public class _GMIPList
    {
        public int ID { get; set; }
        public string IP { get; set; } = string.Empty;
    }
}
