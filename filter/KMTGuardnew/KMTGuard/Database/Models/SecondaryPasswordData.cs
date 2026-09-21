using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace KMTGuard.Database.Models
{
    public class SecondaryPasswordData
    {
        public int ID { get; set; }
        public string StrUserID { get; set; } = string.Empty;
        public int Password { get; set; }
        public byte[] PasswordHash { get; set; } = Array.Empty<byte>();
        public byte[] PasswordSalt { get; set; } = Array.Empty<byte>();
        public string Hwid { get; set; } = string.Empty;
        public string DeviceKeyThumbprint { get; set; } = string.Empty;
        public bool RememberPC { get; set; }
    }
}
