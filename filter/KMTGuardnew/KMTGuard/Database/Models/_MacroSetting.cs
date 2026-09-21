using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace KMTGuard.Database.Models
{
    public class _MacroSetting
    {
        public int ID { get; set; }
        public int CharID { get; set; }
        public bool AutoPotion { get; set; }
        public bool AutoSkill { get; set; }
        public bool AutoHunt { get; set; }
        public bool AutoPickup { get; set; }
        public bool AutoScroll { get; set; }
    }
}
