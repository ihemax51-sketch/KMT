using System;
using System.Collections.Generic;
using System.Data;
using System.Linq;
using System.Net;
using System.Security.Cryptography;
using System.Text;
using System.Threading.Tasks;
using KMTGuard.Helpers;
using KMTGuard.SessionManager;
using Microsoft.Data.SqlClient;
using SilkroadSecurityAPI;
using KMTGuard.AsyncServerManager;
using KMTGuard.SettingManager;
using KMTGuard.PacketHandlerManager;
using KMTGuard.Database;
using KMTGuard.Server.GatewayPacketHandler;
using KMTGuard.Database.Models;
using KMTGuard.ServerManagers;
using Serilog;
namespace KMTGuard.Server
{
    public class GatewayServer : AsyncServer
    {
        public GatewayServer(__ProxyServices service) : base(service)
        {
            var whitelist = sqlQueryHelper.GetWhitelistAsync(Program.Connectionstring, Convert.ToInt32(ServerType.GatewayServer)).GetAwaiter().GetResult();
            var blacklist = sqlQueryHelper.GetBlacklistAsync(Program.Connectionstring, Convert.ToInt32(ServerType.GatewayServer)).GetAwaiter().GetResult();

            var temp3 = new HashSet<ushort>(whitelist.Select(i => ushort.Parse(i.ToString())));
            var temp4 = new HashSet<ushort>(blacklist.Select(i => ushort.Parse(i.ToString())));
            temp3.Add(0x166A); // client DLL account register request
            temp3.Add(0x1670); // secure quick login create token
            temp3.Add(0x1672); // secure quick login play
            temp3.Add(0x1674); // secure quick login revoke token

            PacketHandler = new PacketHandler(temp3, temp4);

            // ping
            PacketHandler.RegisterClientHandler(0x2002, (packet, session, _) =>
            {
                session.LastPing = DateTime.Now;
                return Task.FromResult(new PacketResult());
            });

            PacketHandler.RegisterModuleHandler(0xA102,
             SERVER_GATEWAY_LOGIN_RESPONSE); // Automatically redirect to the AgentServer
            PacketHandler.RegisterModuleHandler(0xA100,
             SERVER_GATEWAY_PATCH_RESPONSE); // Automatically redirect to the DownloadServer
            PacketHandler.RegisterModuleHandler(0x2322,
            SERVER_GATEWAY_LOGIN_IBUV_CHALLENGE); //replacement captcha
       

            var Client = new SERVER_DLL_SETTINGS_RESPONSE(this, PacketHandler);
        }
        public string getMD5(string str)
        {
            MD5 mD = MD5.Create();
            byte[] array = mD.ComputeHash(Encoding.Default.GetBytes(str));
            StringBuilder stringBuilder = new StringBuilder();
            for (int i = 0; i < array.Length; i++)
            {
                stringBuilder.Append(array[i].ToString("x2"));
            }
            return stringBuilder.ToString();
        }



        public override void AddSession(ISession session)
        {
            try
            {
                ServerManager.GatewaySessions.Add(session);
            }
            //Log.Warning(2, "session added");
            catch (Exception EX)
            {
                Log.Warning(EX.Message.ToString());
            }
        }

        public override void RemoveSession(ISession session)
        {
            //Log.Warning("session removed");
            ServerManager.GatewaySessions.Remove(session);

        }
        public override void Dispose()
        {
            foreach (var gatewaySession in ServerManager.GatewaySessions)
            {
                gatewaySession.Stop("Gateway service shutdown");
            }
            base.Dispose();
        }

        private static string GetClientFacingAddress(string bindAddress, string clientAddress)
        {
            if (string.IsNullOrWhiteSpace(bindAddress))
                return Program.MainMachineIP;

            if (!IPAddress.TryParse(bindAddress, out var ipAddress))
                return bindAddress;

            if (IPAddress.Any.Equals(ipAddress) || IPAddress.Loopback.Equals(ipAddress))
                return Program.MainMachineIP;

            if (IsPrivateIPv4(ipAddress) &&
                IPAddress.TryParse(clientAddress, out var clientIp) &&
                !IsPrivateIPv4(clientIp) &&
                !string.IsNullOrWhiteSpace(Program.MainMachineIP))
            {
                return Program.MainMachineIP;
            }

            return bindAddress;
        }

        private static bool IsPrivateIPv4(IPAddress ipAddress)
        {
            if (ipAddress.AddressFamily != System.Net.Sockets.AddressFamily.InterNetwork)
                return false;

            var bytes = ipAddress.GetAddressBytes();
            return bytes[0] == 10 ||
                   (bytes[0] == 172 && bytes[1] >= 16 && bytes[1] <= 31) ||
                   (bytes[0] == 192 && bytes[1] == 168) ||
                   bytes[0] == 127;
        }

        private Task<PacketResult> SERVER_GATEWAY_PATCH_RESPONSE(Packet packet, ISession session, object obj)
        {
            var result = packet.ReadUInt8(); // 1	byte	result
            if (result != 0x02)
            {
                return Task.FromResult(new PacketResult(PacketResultType.Nothing));
            }

            var errorCode = packet.ReadUInt8(); // 1	byte	errorCode
            if (errorCode != 0x02)
            {
                return Task.FromResult(new PacketResult(PacketResultType.Nothing));
            }

            var bindPort = 0;
            var bindAddress = "0.0.0.0";

            var downloadServerIp = packet.ReadAscii(); // *	string	DownloadServer.IP
            var downloadServerPort = packet.ReadUInt16(); //2	ushort	DownloadServer.Port
            var downloadServerCurVersion = packet.ReadUInt32(); //4	uint	DownloadServer.CurVersion

            var configuredDownload = ServerManager.FindConfiguredService(
                ServerType.DownloadServer,
                downloadServerIp,
                downloadServerPort);
            if (configuredDownload != null)
            {
                bindAddress = configuredDownload.BindIP;
                bindPort = configuredDownload.BindPort;
            }

            if (bindPort == 0)
            {
                var downloadServer = ServerManager.FindConfiguredService(ServerType.DownloadServer);
                if (downloadServer != null)
                {
                    bindAddress = downloadServer.BindIP;
                    bindPort = downloadServer.BindPort;
                    Log.Warning("Gateway patch redirect fallback used. Original download target was {DownloadIp}:{DownloadPort}, fallback is {BindIp}:{BindPort}",
                        downloadServerIp, downloadServerPort, bindAddress, bindPort);
                }
            }

            if (bindPort == 0)
            {
                Log.Warning("Gateway patch response passthrough for {ClientIp}: no DownloadServer proxy matched {DownloadIp}:{DownloadPort}",
                    session.ClientIp, downloadServerIp, downloadServerPort);
                return Task.FromResult(new PacketResult(PacketResultType.Nothing));
            }

            var overridePacket = new Packet(0xA100, packet.Encrypted, packet.Massive);
            overridePacket.WriteUInt8(result);
            overridePacket.WriteUInt8(errorCode);

            var clientFacingAddress = GetClientFacingAddress(bindAddress, session.ClientIp);
            overridePacket.WriteAscii(clientFacingAddress);
            overridePacket.WriteUInt16(bindPort);
            overridePacket.WriteUInt32(downloadServerCurVersion);

            while (true)
            {
                var hasEntries = packet.ReadBool(); // 1	bool hasEntries
                overridePacket.WriteBool(hasEntries);
                if (!hasEntries)
                    break;


                overridePacket.WriteUInt32(packet.ReadUInt32()); //4	uint	file.ID
                overridePacket.WriteAscii(packet.ReadAscii()); //    *	string	file.Name
                overridePacket.WriteAscii(packet.ReadAscii()); //    *	string	file.Path
                overridePacket.WriteUInt32(packet.ReadUInt32()); //4	uint	file.Length //in bytes
                overridePacket.WriteBool(packet.ReadBool()); //1	bool	file.ToBePacked //into pk2
            }

            return Task.FromResult(new PacketResult(overridePacket, PacketResultType.Override));
        }

        private async Task<PacketResult> SERVER_GATEWAY_LOGIN_RESPONSE(Packet packet, ISession session, object obj)
        {
            var flag = packet.ReadUInt8();
            // Block everything else
            if (flag != 0x01)
            {
                QuickLoginAgentAuthBridge.CancelGatewayLogin(session.ClientGuid);
                RestoreAuthenticationAfterFailedLogin(session);
                Log.Warning("Gateway login response failed for {ClientIp}; flag=0x{Flag:X2}", session.ClientIp, flag);
                return new PacketResult();
            }

            var token = packet.ReadUInt32();
            var remoteAddress = packet.ReadAscii(); // host
            var remotePort = packet.ReadUInt16(); // host
            var quickLoginBinding = await QuickLoginAgentAuthBridge.BindGatewayTokenAsync(
                session.ClientGuid,
                token,
                session.ClientIp);
            if (quickLoginBinding.Success)
            {
                Log.Information("Secure quick login agent auth bridged for {Username} from {ClientIp}", quickLoginBinding.Username, session.ClientIp);
            }

            var bindPort = 0;
            var bindAddress = "0.0.0.0";

            // finds the according agentServer to the given port and address and writes the redirect packet
            var configuredAgent = ServerManager.FindConfiguredService(
                ServerType.AgentServer,
                remoteAddress,
                remotePort);
            if (configuredAgent != null)
            {
                bindAddress = configuredAgent.BindIP;
                bindPort = configuredAgent.BindPort;
            }

            if (bindPort == 0)
            {
                var agentServer = ServerManager.FindConfiguredService(ServerType.AgentServer);
                if (agentServer != null)
                {
                    bindAddress = agentServer.BindIP;
                    bindPort = agentServer.BindPort;
                    Log.Warning("Gateway login redirect fallback used. Original agent target was {AgentIp}:{AgentPort}, fallback is {BindIp}:{BindPort}",
                        remoteAddress, remotePort, bindAddress, bindPort);
                }
            }

            // create a new Packet and override the old client before sending to client
            var redirectPacket = new Packet(0xA102, true);
            redirectPacket.WriteUInt8(0x01);
            redirectPacket.WriteUInt32(token);
            var clientFacingAgentAddress = GetClientFacingAddress(bindAddress, session.ClientIp);
            redirectPacket.WriteAscii(clientFacingAgentAddress);
            redirectPacket.WriteUInt16(bindPort);


            ServerManager.g_DelayedJobMgr.CreateJob(new DelayedJobItem(
                2000,
                session,
                string.Empty,
                (state, _) =>
                {
                    ((ISession)state).Stop("gateway redirect completed");
                    return Task.CompletedTask;
                }));
            return new PacketResult(redirectPacket, PacketResultType.Override);
        }

        internal static void RestoreAuthenticationAfterFailedLogin(ISession session)
        {
            session.PendingQuickLogin = false;
            session.PendingPrimaryLogin = false;
            session.PendingPrimaryLoginStartedAt = 0;

            var hasVerifiedDevice =
                !string.IsNullOrWhiteSpace(session.SessionData.Hwid) &&
                !string.IsNullOrWhiteSpace(session.DeviceKeyThumbprint) &&
                !string.IsNullOrWhiteSpace(session.DevicePublicKey);

            session.GatewayAuthenticationState = hasVerifiedDevice
                ? GatewayAuthenticationState.AwaitingPrimaryCredentials
                : GatewayAuthenticationState.AwaitingHwid;
        }
        private async Task<PacketResult> SERVER_GATEWAY_LOGIN_IBUV_CHALLENGE(Packet packet, ISession session, object obj)
        {

            //if remove captcha is not enabled return nothing
            if (!_serverSettings.RemoveCaptcha)
            {
                return new PacketResult();
            }

            var removePacket = new Packet(0x6323, false);
            removePacket.WriteAscii(_serverSettings.CaptchaValue);
            await session.SendToServer(removePacket);
            return new PacketResult(PacketResultType.Block);
        }


    }
}
