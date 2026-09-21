using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace KMTGuard.Database.Models
{
    public class _RefMapSettings
    {
        public string EventName { get; set; } = string.Empty;
        public byte RegionType { get; set; }
        public int RegionID { get; set; }
        public bool EventSuit { get; set; }
        public bool HideBuffViewer { get; set; }
        public bool DisablePetSpawn { get; set; }
        public bool DisableParty { get; set; }
        public bool AutoCape { get; set; }
        public bool DisableChat { get; set; }
        public bool DisableTrace { get; set; }
        public bool DisableZerk { get; set; }
        public bool ClosePlayerAttack { get; set; }
        public bool HideMiniMap { get; set; }
        public bool HideName { get; set; }
    }
}
