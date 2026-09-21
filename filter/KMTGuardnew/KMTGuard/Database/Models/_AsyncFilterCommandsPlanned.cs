using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace KMTGuard.Database.Models
{
    public class _AsyncFilterCommandsPlanned
    {
        public int ID { get; set; }
        public int CommandID { get; set; }
        public string? Data1 { get; set; }
        public string? Data2 { get; set; }
        public string? Data3 { get; set; }
        public string? Data4 { get; set; }
        public string? Data5 { get; set; }
        public string? Data6 { get; set; }
        public string? Data7 { get; set; }
        public string? Data8 { get; set; }
        public string? Data9 { get; set; }
        public string? Data10 { get; set; }
        public string? Data11 { get; set; }
        public byte Status { get; set; }
        public Guid? ClaimToken { get; set; }
        public Guid? IdempotencyKey { get; set; }
        public int Attempts { get; set; }
    }
}
