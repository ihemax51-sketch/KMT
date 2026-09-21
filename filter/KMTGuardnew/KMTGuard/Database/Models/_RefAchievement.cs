using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace KMTGuard.Database.Models
{
    public class _RefAchievement
    {
        public int ID { get; set; }
        public bool Service { get; set; }
        public byte Category { get; set; }
        public string Name { get; set; } = string.Empty;
        public byte RewardType { get; set; }
        public byte RewardTagID { get; set; }
        public int RewardSkillPoint { get; set; }
        public Int64 RewardGold { get; set; }
    }
}
