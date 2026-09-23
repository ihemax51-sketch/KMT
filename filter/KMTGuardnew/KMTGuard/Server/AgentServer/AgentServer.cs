using System;
using System.Collections.Generic;
using System.Data;
using System.Data.Common;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Dapper;
using KMTGuard.AsyncServerManager;
using KMTGuard.Clientless;
using KMTGuard.Database;
using KMTGuard.Database.Models;
using KMTGuard.Helpers;
using KMTGuard.PacketHandlerManager;
using KMTGuard.Server.AgentPacketHandler;
using KMTGuard.ServerManagers;
using KMTGuard.Servers.PacketHandler;
using KMTGuard.SessionManager;
using Microsoft.Data.SqlClient;
using SilkroadSecurityAPI;


namespace KMTGuard.Server
{
    public class AgentServer : AsyncServer
    {
        public AgentServer(__ProxyServices service) : base(service)
        {
            var whitelist = sqlQueryHelper.GetWhitelistAsync(Program.Connectionstring, Convert.ToInt32(ServerType.AgentServer)).GetAwaiter().GetResult();
            var blacklist = sqlQueryHelper.GetBlacklistAsync(Program.Connectionstring, Convert.ToInt32(ServerType.AgentServer)).GetAwaiter().GetResult();

            var temp3 = new HashSet<ushort>(whitelist.Select(i => ushort.Parse(i.ToString())));
            var temp4 = new HashSet<ushort>(blacklist.Select(i => ushort.Parse(i.ToString())));
            temp3.Add(OfflineStallService.ActivateRequestOpcode);
            // ActionWnd commands are consumed by CustomUIPackets.  They must
            // be whitelisted before packet dispatch; registration alone does
            // not bypass the client packet whitelist.
            temp3.Add(0xCC1E);
            temp3.Add(ClientlessTeleportProtocol.ClientTeleportReadyResponse);
            temp3.Add(ClientlessTeleportProtocol.AlternateClientTeleportReadyResponse);
            temp4.Add(0x35FE);

            PacketHandler = new PacketHandler(temp3, temp4);

            // ping
            PacketHandler.RegisterClientHandler(0x2002, async (packet, session, _) =>
            {
                session.LastPing = DateTime.Now;
                if (await BotProtectionService.ShouldDisconnectBotSessionAsync(session))
                {
                    session.Stop("Bot login is disabled");
                    return new PacketResult(PacketResultType.Disconnect);
                }

                await RegionControlService.CheckInactivityAsync(session);
                if (session.IsStopped)
                    return new PacketResult(PacketResultType.Disconnect);

                await RegionControlService.ApplyAutoPvpAsync(session);
                return new PacketResult();
            });
            PacketHandler.RegisterClientHandler(0x7021, async (packet, session, _) =>
            {
                var freezeBlock = await TeleportFreezeService.BlockMovementIfFrozenAsync(session);
                if (freezeBlock != null)
                    return freezeBlock;

                var blocked = await RegionControlService.BlockIfAsync(
                    session,
                    rule => !rule.Enable_Move,
                    "Region.MovementDisabled");

                if (blocked != null)
                    return blocked;

                RegionControlService.MarkMovement(session);
                await RegionControlService.ApplyAutoPvpAsync(session);
                return new PacketResult();
            });
            var academy = new AcademyPackets(this, PacketHandler);
            var Alchemy = new AlchemyPackets(this, PacketHandler);
            var charaction = new CharAction(this, PacketHandler);
            var chardata = new CharDataPackets(this, PacketHandler);
            var charsel = new CharacterSelection(this, PacketHandler);
            var chatp = new ChatPackets(this, PacketHandler);
            var COS = new COSPackets(this, PacketHandler);
            var GS = new CustomGameServerPacketHandler(this, PacketHandler);
            var guild = new GuildPackets(this, PacketHandler);
            var inv = new InventoryPackets(this, PacketHandler);
            var job = new JobPackets(PacketHandler);
            var ptdata = new PartyData(this, PacketHandler);
            var pvpcape = new PvpCapePackets(this, PacketHandler);
            var stall = new StallPackets(this, PacketHandler);
            var Client = new ExploitFixPackets(this, PacketHandler);
            var ClientPacket = new CustomUIPackets(this, PacketHandler);
            
        }

        public async Task BroadcastPacketbyWorldIDAndLayerID(int WorldID, int LayerID, Packet packet, bool clientIsReady = true)
        {
            var targets = ServerManager.AgentSessions
                .Where(s => ServerManager.IsClientDeliveryTarget(s, clientIsReady) &&
                            s.SessionData.WorldID == WorldID &&
                            s.SessionData.WorldLayerID == LayerID);
            packet.ToReadOnly();
            await Task.WhenAll(targets.Select(s => s.SendToClient(packet.CreateReadOnlyClone())));
        }

        public async Task BroadcastPacket(Packet packet, ServerType serverType = ServerType.AgentServer, bool clientIsReady = true)
        {
            var targets = ServerManager.AgentSessions
                .Where(s => ServerManager.IsClientDeliveryTarget(s, clientIsReady));
            packet.ToReadOnly();
            await Task.WhenAll(targets.Select(s => s.SendToClient(packet.CreateReadOnlyClone())));
        }
        public async Task BroadcastPacketToCharName(string CharName, Packet packet, bool clientIsReady = true)
        {
            var targets = ServerManager.AgentSessions
                .Where(s => ServerManager.IsClientDeliveryTarget(s, clientIsReady) &&
                            s.SessionData.Charname == CharName);
            packet.ToReadOnly();
            await Task.WhenAll(targets.Select(s => s.SendToClient(packet.CreateReadOnlyClone())));
        }
        public async Task BroadcastPacketbyWorldID(int WorldID, Packet packet, bool clientIsReady = true)
        {
            var targets = ServerManager.AgentSessions
                .Where(s => ServerManager.IsClientDeliveryTarget(s, clientIsReady) &&
                            s.SessionData.WorldID == WorldID);
            packet.ToReadOnly();
            await Task.WhenAll(targets.Select(s => s.SendToClient(packet.CreateReadOnlyClone())));
        }
        public async Task BroadcastPacketbyRegionID(int Region, Packet packet, bool clientIsReady = true)
        {
            var targets = ServerManager.AgentSessions
                .Where(s => ServerManager.IsClientDeliveryTarget(s, clientIsReady) &&
                            s.SessionData.LatestRegion == Region);
            packet.ToReadOnly();
            await Task.WhenAll(targets.Select(s => s.SendToClient(packet.CreateReadOnlyClone())));
        }
        public async Task<bool> TryGetItemInfoAsync(SItemInfoDbRecord stResult, int nCharID, byte btSlotIndex)
        {
            string query = "EXEC [dbo].[Item_GetInfo] @CharID, @SlotIndex, @RefItemID OUTPUT, @OptLevel OUTPUT, @CodeName OUTPUT, @ItemDBID OUTPUT, @AdvOptLevel OUTPUT";

            using (var connection = new SqlConnection(Program.Connectionstring))
            {
                using (var command = new SqlCommand(query, connection))
                {
                    command.Parameters.AddWithValue("@CharID", nCharID);
                    command.Parameters.AddWithValue("@SlotIndex", btSlotIndex);

                    var refItemIDParam = new SqlParameter("@RefItemID", SqlDbType.Int) { Direction = ParameterDirection.Output };
                    var optLevelParam = new SqlParameter("@OptLevel", SqlDbType.TinyInt) { Direction = ParameterDirection.Output };
                    var codeNameParam = new SqlParameter("@CodeName", SqlDbType.VarChar, 128) { Direction = ParameterDirection.Output };
                    var itemDBIDParam = new SqlParameter("@ItemDBID", SqlDbType.Int) { Direction = ParameterDirection.Output };
                    var advOptLevelParam = new SqlParameter("@AdvOptLevel", SqlDbType.TinyInt) { Direction = ParameterDirection.Output };

                    command.Parameters.Add(refItemIDParam);
                    command.Parameters.Add(optLevelParam);
                    command.Parameters.Add(codeNameParam);
                    command.Parameters.Add(itemDBIDParam);
                    command.Parameters.Add(advOptLevelParam);

                    //Log.Warning("Query: " + query);
                    //Log.Warning("Parameters: CharID={0}, SlotIndex={1}", nCharID, btSlotIndex);

                    await connection.OpenAsync();
                    //Log.Warning("Bağlantı başarıyla açıldı.");

                    await command.ExecuteNonQueryAsync();

                    stResult.nRefItemID = (int)refItemIDParam.Value;
                    stResult.btOptLevel = (byte)optLevelParam.Value;
                    stResult.szCodeName = (string)codeNameParam.Value;
                    stResult.nItemDBID = (int)itemDBIDParam.Value;
                    stResult.btAdvOptLevel = (byte)advOptLevelParam.Value;

                    //Log.Warning("Results: nRefItemID={0}, btOptLevel={1}, szCodeName={2}, nItemDBID={3}, btAdvOptLevel={4}",
                    //    stResult.nRefItemID, stResult.btOptLevel, stResult.szCodeName, stResult.nItemDBID, stResult.btAdvOptLevel);

                    // Değerlerin null olup olmadığını kontrol etme
                    if (refItemIDParam.Value != DBNull.Value && optLevelParam.Value != DBNull.Value &&
                        codeNameParam.Value != DBNull.Value && itemDBIDParam.Value != DBNull.Value &&
                        advOptLevelParam.Value != DBNull.Value)
                    {
                        //Log.Warning("Tüm parametreler başarılı şekilde dolduruldu.");
                        return true;
                    }
                    else
                    {
                        //Log.Warning("Bir veya daha fazla parametre null değeri döndü.");
                        return false;
                    }
                }
            }
        }


        public Task UpdateTimers(object state)
        {
            // CreatedTimerListWorldID koleksiyonunu kontrol et
            return Task.CompletedTask;
        }
      
        public override void AddSession(ISession session)
        {
            ServerManager.AgentSessions.Add(session);
        }

        public override void RemoveSession(ISession session)
        {
            ServerManager.AgentSessions.Remove(session);
        }

        public override void Dispose()
        {
            foreach (var agentSession in ServerManager.AgentSessions.ToArray())
                agentSession.Stop("Agent service shutdown");

            base.Dispose();
        }
       public string FormatNumber(long num)
        {
            if (num >= 100000000)
            {
                return (num / 1000000D).ToString("0.#M");
            }
            if (num >= 1000000)
            {
                return (num / 1000000D).ToString("0.##M");
            }
            if (num >= 100000)
            {
                return (num / 1000D).ToString("0.#k");
            }
            if (num >= 10000)
            {
                return (num / 1000D).ToString("0.##k");
            }

            return num.ToString("#,0");
        }
    }
}
