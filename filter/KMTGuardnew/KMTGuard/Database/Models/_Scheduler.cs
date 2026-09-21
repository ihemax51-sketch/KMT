using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace KMTGuard.Database.Models;

public class _Scheduler
{
    public int Idx { get; set; }
    public string Name { get; set; } = string.Empty;
    public int Day { get; set; }
    public TimeSpan Time { get; set; }
    public string Query { get; set; } = string.Empty;
    public bool IsEnabled { get; set; }
    public string Comment { get; set; } = string.Empty;
    public bool IsRunning { get; set; } = false;

}
