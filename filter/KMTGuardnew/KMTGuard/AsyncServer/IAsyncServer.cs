using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Sockets;
using System.Net;
using System.Text;
using System.Threading.Tasks;
using KMTGuard.Helpers;
using KMTGuard.PacketHandlerManager;
using KMTGuard.SessionManager;
using KMTGuard.Database.Models;
namespace KMTGuard.AsyncServerManager
{
    public interface IAsyncServer : IDisposable
    {
        __ProxyServices Service { get; init; }
        bool Exit { get; set; }
        bool Started { get; set; }

        TcpListener _tcpServer { get; set; }
        IPacketHandler PacketHandler { get; set; }
        IPEndPoint RemoteEndPoint { get; set; }
        void AddSession(ISession session);
        void RemoveSession(ISession session);
        Task Start();
        void Stop();
        Task OnAccept(Task<TcpClient> task);
        public Task<int> GetHWIDCount(string hwid);
    }
}
