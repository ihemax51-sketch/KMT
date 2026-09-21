using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using SilkroadSecurityAPI;

namespace XFilterV2.Helpers
{
    public enum E_PARTY_UPDATE_TYPE : byte
    {
        DISMISSED = 1,
        MEMBER_JOINED = 2,
        MEMBER_LEFT = 3,
        MEMBER_INFO = 6,
        LEADER_CHANGE = 9
    }
    public enum E_PARTY_LEAVE_TYPE : byte
    {
        DISCONNECTED = 1,
        LEFT = 2,
        BANNED = 3
    }
    public enum E_PARTY_SETTINGS_FLAG
    {
        NONE = 0, // Diğer ayar bayrakları burada eklenebilir.
    }

    public class SPartyMemberInfo
    {
        public uint nMemberID { get; set; }
        public string strName { get; set; }
        public uint nRefObjID { get; set; }
        public byte btLevel { get; set; }
        public byte btHpMana { get; set; }
        public SPosInfo stPos { get; set; }
        public string strGuildName { get; set; }
        public uint nMasteryId1 { get; set; }
        public uint nMasteryId2 { get; set; }

        public SPartyMemberInfo()
        {
            nMemberID = 0;
            strName = string.Empty;
            nRefObjID = 0;
            btLevel = 0;
            btHpMana = 0;
            stPos = new SPosInfo();
            strGuildName = string.Empty;
            nMasteryId1 = 0;
            nMasteryId2 = 0;
        }
    }

    public class SPartyInfo
    {
        public uint nPartyID { get; set; }
        public uint nLeaderID { get; set; }
        public E_PARTY_SETTINGS_FLAG eSettingFlags { get; set; }
        public List<SPartyMemberInfo> vMembers { get; set; }

        public SPartyInfo()
        {
            nPartyID = 0;
            nLeaderID = 0;
            eSettingFlags = E_PARTY_SETTINGS_FLAG.NONE;
            vMembers = new List<SPartyMemberInfo>();
        }
    }
    public class SPosInfo
    {
        public short ShRegionID { get; set; }
        public float FPosX { get; set; }
        public float FPosY { get; set; }
        public float FPosZ { get; set; }
        public ushort WWorldID { get; set; }
        public ushort WWorldLayerID { get; set; }

        public SPosInfo()
        {
            ShRegionID = 0;
            FPosX = 0.0f;
            FPosY = 0.0f;
            FPosZ = 0.0f;
            WWorldID = 0;
            WWorldLayerID = 0;
        }

        public SPosInfo(short shRegionID, float fPosX, float fPosY, float fPosZ, ushort wWorldID, ushort wWorldLayerID)
        {
            ShRegionID = shRegionID;
            FPosX = fPosX;
            FPosY = fPosY;
            FPosZ = fPosZ;
            WWorldID = wWorldID;
            WWorldLayerID = wWorldLayerID;
        }

        public bool IsNormalWorld()
        {
            return (ShRegionID & 0x8000) == 0;
        }

        public bool IsDungeon()
        {
            return !IsNormalWorld();
        }

        public void ParseFromMsg(Packet stMsg, bool bParseWorldInfo)
        {
            ShRegionID = stMsg.ReadInt16();

            if (IsNormalWorld())
            {
                FPosX = stMsg.ReadInt16();
                FPosY = stMsg.ReadInt16();
                FPosZ = stMsg.ReadInt16();
            }
            else
            {
                FPosX = stMsg.ReadFloat();
                FPosY = stMsg.ReadFloat();
                FPosZ = stMsg.ReadFloat();
            }

            if (bParseWorldInfo)
            {
                WWorldID = stMsg.ReadUInt16();
                WWorldLayerID = stMsg.ReadUInt16();
            }
            else
            {
                WWorldID = 0;
                WWorldLayerID = 0;
            }
        }
    }
}