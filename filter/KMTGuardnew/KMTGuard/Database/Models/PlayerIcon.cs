using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace KMTGuard.Database.Models
{
    public class PlayerIcon
    {
        public int ID { get; set; }
        public int CharID { get; set; }
        public int IconID { get; set; }
        public byte Side { get; set; }

    }
}
