using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace KMTGuard.Database.Models
{
	public class _MacroAutoPotion
	{
		public int ID { get; set; }
		public int CharID { get; set; }
		public byte Slot { get; set; }
		public bool Active { get; set; }
		public byte Value { get; set; }
	}
}
