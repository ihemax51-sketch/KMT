using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace KMTGuard.Database.Models
{
    public class _AchievementCondition
    {
        public int ID { get; set; }
        public int CharID { get; set;}
        public int PlayerAchievementID { get; set;}
        public int RefAchievementID { get; set;}
        public int RefAchievementConditionID { get; set;}
        public Int64 ProgressCount { get; set;}
    }
}
