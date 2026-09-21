using System;
using System.Collections.Generic;
using System.Data;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using KMTGuard.AsyncServerManager;
using KMTGuard.Database;
using KMTGuard.Database.Models;
using KMTGuard.Helpers;
using KMTGuard.PacketHandlerManager;
using KMTGuard.ServerManagers;
using KMTGuard.SessionManager;
using Microsoft.Data.SqlClient;

namespace KMTGuard.Server.DownloadServer
{
    public class DownloadServer : AsyncServer
    {
        public DownloadServer(__ProxyServices service) : base(service)
        {

            var whitelist = sqlQueryHelper.GetWhitelistAsync(Program.Connectionstring, Convert.ToInt32(ServerType.DownloadServer)).GetAwaiter().GetResult();
            var blacklist = sqlQueryHelper.GetBlacklistAsync(Program.Connectionstring, Convert.ToInt32(ServerType.DownloadServer)).GetAwaiter().GetResult();

            var temp3 = new HashSet<ushort>(whitelist.Select(i => ushort.Parse(i.ToString())));
            var temp4 = new HashSet<ushort>(blacklist.Select(i => ushort.Parse(i.ToString())));

            PacketHandler = new PacketHandler(temp3, temp4);

            // ping
            PacketHandler.RegisterClientHandler(0x2002, (packet, session, _) =>
            {
                session.LastPing = DateTime.Now;
                return Task.FromResult(new PacketResult());
            });
        }
        public override void AddSession(ISession session)
        {
            ServerManager.DownloadSessions.Add(session);
        }

        public override void RemoveSession(ISession session)
        {
            ServerManager.DownloadSessions.Remove(session);
        }

        public override void Dispose()
        {
            foreach (var downloadSession in ServerManager.DownloadSessions)
            {
                downloadSession.Stop("Download service shutdown");
            }
            base.Dispose();
        }
    }
}
