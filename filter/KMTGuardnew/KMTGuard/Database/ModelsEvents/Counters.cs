using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace KMTGuard.Database.ModelsEvents
{
    public class TimerWorldandPlayer
    {
        public int WorldID { get; set; }
        public int LayerID { get; set; }
    }
    public class SFortressWarCounter
    {
        public string CharName { get; set; } = string.Empty;
        public string GuildName { get; set; } = string.Empty;
        public string UnionName { get; set; } = string.Empty;
        public int WorldID { get; set; }
        public int Kill { get; set; }
    };
}
