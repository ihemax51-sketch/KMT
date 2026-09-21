using Dapper;
using Microsoft.Data.SqlClient;
using Serilog;
using System;
using System.Collections.Generic;
using System.Data;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace KMTGuard.ServerManagers
{
    internal class TriggerService
    {
        static readonly SemaphoreSlim _sqlGate = new(16, 16);

        public static async Task<bool> ExecuteBlockCheckSP(string procName, object parameters)
        {
            var acquired = false;
            try
            {
                acquired = await _sqlGate.WaitAsync(TimeSpan.FromSeconds(3));
                if (!acquired)
                {
                    Log.Warning("Trade trigger {ProcName} rejected: SQL concurrency gate timeout", procName);
                    return true;
                }

                await using var con = new SqlConnection(Program.Connectionstring);
                await con.OpenAsync();
                var dynamicParams = new DynamicParameters(parameters);
                dynamicParams.Add("@IsBlocked", dbType: DbType.Boolean, direction: ParameterDirection.Output);
                await con.ExecuteAsync(procName, dynamicParams, commandType: CommandType.StoredProcedure,
                    commandTimeout: 10);
                return dynamicParams.Get<bool?>("@IsBlocked") == true;
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "Trade trigger {ProcName} failed; action rejected", procName);
                return true;
            }
            finally
            {
                if (acquired)
                    _sqlGate.Release();
            }
        }

        #region trade triggers
        public static Task<bool> ControlTradeGoodsBuyingRequest(int charID, string charname, int npcID, string npcCodename, byte npcTab, byte npcSlot, short quantity, byte jobStatus, string hwid) =>
            ExecuteBlockCheckSP("[dbo].[_CustomTradeGoodsBuyingRequest]", new
            {
                CharID = charID,
                Charname = charname,
                NpcID = npcID,
                NpcCodename = npcCodename,
                NpcTab = npcTab,
                NpcSlot = npcSlot,
                Quantity = quantity
            });

        public static Task<bool> ControlTradeGoodsSellingRequest(int charID, string charname, int petid, int npcID, string npcCodename, byte petSlot, short quantity, byte jobStatus, string hwid) =>
            ExecuteBlockCheckSP("[dbo].[_CustomTradeGoodsSellingRequest]", new
            {
                CharID = charID,
                Charname = charname,
                Petid = petid,
                NpcID = npcID,
                NpcCodename = npcCodename,
                PetSlot = petSlot,
                Quantity = quantity
            });

        public static async Task CompleteTradeGoodsBuying(int charID, string charname, int petid, int npcID, string npcCodename, byte npcTab, byte npcSlot, short quantity, byte jobStatus, string hwid)
        {
            var acquired = false;
            try
            {
                acquired = await _sqlGate.WaitAsync(TimeSpan.FromSeconds(3));
                if (!acquired)
                    throw new TimeoutException("Trade completion SQL concurrency gate timed out.");
                await using (var connection = new SqlConnection(Program.Connectionstring))
                {
                    await connection.OpenAsync();
                    await connection.ExecuteAsync("[dbo].[_CustomTradeGoodsBuying]", new
                        {
                            CharID = charID,
                            Charname = charname,
                            Petid = petid,
                            NpcID = npcID,
                            NpcCodename = npcCodename,
                            NpcTab = npcTab,
                            NpcSlot = npcSlot,
                            Quantity = quantity
                        }, commandType: CommandType.StoredProcedure, commandTimeout: 10);
                }
            }
            catch (Exception ex)
            {
                Log.Error(ex, "Trade goods buying completion failed closed for CharID {CharID}", charID);
                throw;
            }
            finally
            {
                if (acquired)
                    _sqlGate.Release();
            }
        }

        public static async Task CompleteTradeGoodsSelling(int charID, string charname, int petid, int npcID, string npcCodename, byte petSlot, short quantity, byte jobStatus, string hwid)
        {
            var acquired = false;
            try
            {
                acquired = await _sqlGate.WaitAsync(TimeSpan.FromSeconds(3));
                if (!acquired)
                    throw new TimeoutException("Trade completion SQL concurrency gate timed out.");
                await using (var connection = new SqlConnection(Program.Connectionstring))
                {
                    await connection.OpenAsync();
                    await connection.ExecuteAsync("[dbo].[_CustomTradeGoodsSelling]", new
                        {
                            CharID = charID,
                            Charname = charname,
                            Petid = petid,
                            NpcID = npcID,
                            NpcCodename = npcCodename,
                            PetSlot = petSlot,
                            Quantity = quantity
                        }, commandType: CommandType.StoredProcedure, commandTimeout: 10);
                }
            }
            catch (Exception ex)
            {
                Log.Error(ex, "Trade goods selling completion failed closed for CharID {CharID}", charID);
                throw;
            }
            finally
            {
                if (acquired)
                    _sqlGate.Release();
            }
        }

        #endregion
    }
}
