using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace KMTGuard.Database.Models
{
    public class _RefFellowData
    {
        public int ID { get; set; }
        public int ItemID { get; set; }
        public string PetNameStrID { get; set; } = string.Empty;
        public int SkillID_1 { get; set; }
        public byte Active_Level_1 { get; set; }
        public byte SkillType_1 { get; set; }
        public int SkillID_2 { get; set; }
        public byte Active_Level_2 { get; set; }
        public byte SkillType_2 { get; set; }
        public int SkillID_3 { get; set; }
        public byte Active_Level_3 { get; set; }
        public byte SkillType_3 { get; set; }
        public int SkillID_4 { get; set; }
        public byte Active_Level_4 { get; set; }
        public byte SkillType_4 { get; set; }
        public int SkillID_5 { get; set; }
        public byte Active_Level_5 { get; set; }
        public byte SkillType_5 { get; set; }
        public int SkillAnimationID { get; set; }
        public int SelfSkill_1 { get; set; }
        public byte SelfSkill_Active_Level_1 { get; set; }
        public int SelfSkill_2 { get; set; }
        public byte SelfSkill_Active_Level_2 { get; set; }
    }
}
