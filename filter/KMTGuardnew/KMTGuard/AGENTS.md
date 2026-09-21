# KMTGuard Agent Instructions

Before changing any player-facing Filter message, custom UI response, event
announcement, warning, scoreboard title, or `System_Notices` integration, read:

`../../../docs/player_localization_reference.md`

All Filter-generated player wording must resolve through
`KMTGuard.Localization.PlayerLanguage`. Never add a new hardcoded
player-facing string to a packet handler or service.

Before changing packet handlers, packet logs, client custom opcodes, macro logic, stall logic, silk systems, inventory handling, teleport/reverse scroll handling, or disconnect debugging, read:

`docs/agent-opcodes-reference.md`

Use that file as the local AgentServer opcode map. Always match packets by direction (`C->S` or `S->C`) and gameplay context, not by opcode number alone.

Do not reuse a listed Agent opcode for a custom feature unless the payload is intentionally compatible with the original game protocol.
