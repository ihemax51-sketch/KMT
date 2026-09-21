using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection.PortableExecutable;
using System.Text;
using System.Threading.Tasks;
using KMTGuard.Helpers;

namespace KMTGuard.Database.Models;
public partial class __ProxyServices
{
    public int ServiceId { get; set; }

    public string Name { get; set; } = null!;

    public ServerType ServerType { get; set; }

    public string RemoteIP { get; set; } = null!;
    public int RemotePort { get; set; }
    public string BindIP { get; set; } = null!;

    public int BindPort { get; set; }

    public int ByteLimitation { get; set; }
    public bool AutoStart { get; set; }
}