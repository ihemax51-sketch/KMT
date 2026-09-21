using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace KMTGuard.Database.Models
{
    public class _RefNewAvatarMall
    {
        public int ID { get; set; }
        public bool Service { get; set; }
        public byte CategoryID { get; set; }
        public string CodeName128 { get; set; } = string.Empty;
        public int ItemID { get; set; }
        public int Silk { get; set; }
        public int ItemIndex { get; set; }
        public int PetObjID { get; set; }
    }
}
