using System;
using System.Collections.Generic;
using System.ComponentModel.DataAnnotations;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace KMTGuard.Database.Models
{
    public class _RefHWANLevel
    {
        public byte HwanLevel { get; set; }
        public int ParamFourcc1 { get; set; }
        public byte ParamValue1 { get; set; }
        public int ParamFourcc2 { get; set; }
        public byte ParamValue2 { get; set; }
        public int ParamFourcc3 { get; set; }
        public byte ParamValue3 { get; set; }
        public int ParamFourcc4 { get; set; }
        public byte ParamValue4 { get; set; }
        public int ParamFourcc5 { get; set; }
        public byte ParamValue5 { get; set; }
        public string AssocFileObj128 { get; set; } = string.Empty;
        public string Title_CH70 { get; set; } = string.Empty;
        public string Title_EU70 { get; set; } = string.Empty;
    }
}
