using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace KMTGuard.Database.Models
{
	public class _NewReverseSavedLocations
	{
		public int ID { get; set; }
		public int CharID { get; set; }
		public byte LocationID { get; set; }
		public int RegionID { get; set; }
		public int PosX { get; set; }
		public int PosY { get; set; }
		public int PosZ { get; set; }
		public int WorldID { get; set; }
	}
}
