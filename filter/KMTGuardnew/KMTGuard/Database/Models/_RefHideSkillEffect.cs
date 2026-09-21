using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace KMTGuard.Database.Models
{
    public class _RefHideSkillEffect
    {
        public int ID { get; set; }
        public int SkillID { get; set; }
        public bool JobMode { get; set; }
        public bool MapSettings { get; set; }
    }
}