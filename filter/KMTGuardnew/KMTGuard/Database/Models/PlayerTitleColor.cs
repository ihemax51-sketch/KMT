using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace KMTGuard.Database.Models
{
    public class PlayerTitleColor
    {
        public int ID { get; set; }
        public int CharID { get; set; }
        public string ColorName { get; set; } = string.Empty;
        public string ColorCode { get; set; } = string.Empty;
    }
}
