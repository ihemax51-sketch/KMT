namespace KMTGuard.Clientless;

internal sealed class ClientlessTeleportProtocol
{
    internal const ushort ServerCharacterDataBegin = 0x34A5;
    internal const ushort ServerCharacterData = 0x3013;
    internal const ushort ServerCharacterDataEnd = 0x34A6;
    internal const ushort ServerTeleportReadyRequest = 0x34B5;
    internal const ushort ClientTeleportReadyResponse = 0x34B6;
    internal const ushort AlternateServerTeleportReadyRequest = 0x35B5;
    internal const ushort AlternateClientTeleportReadyResponse = 0x35B6;
    internal const ushort ClientCharacterConfirmSpawn = 0x3012;

    private bool _awaitingTeleportedCharacterDataEnd;

    public ushort? HandleServerOpcode(ushort opcode)
    {
        if (opcode is ServerTeleportReadyRequest or AlternateServerTeleportReadyRequest)
        {
            _awaitingTeleportedCharacterDataEnd = true;
            return opcode == ServerTeleportReadyRequest
                ? ClientTeleportReadyResponse
                : AlternateClientTeleportReadyResponse;
        }

        if (opcode == ServerCharacterDataEnd && _awaitingTeleportedCharacterDataEnd)
        {
            _awaitingTeleportedCharacterDataEnd = false;
            return ClientCharacterConfirmSpawn;
        }

        return null;
    }

    public static bool IsTeleportReadyResponse(ushort opcode) =>
        opcode is ClientTeleportReadyResponse or AlternateClientTeleportReadyResponse;

    public void Reset()
    {
        _awaitingTeleportedCharacterDataEnd = false;
    }
}
