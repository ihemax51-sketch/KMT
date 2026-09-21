using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace KMTGuard.Database.Models
{
    public class _CharacterSettings
    {
        public int ID { get; set; }
        public string CharName16 { get; set; } = string.Empty;
        public bool HideItemInfo { get; set; }
    }
}
