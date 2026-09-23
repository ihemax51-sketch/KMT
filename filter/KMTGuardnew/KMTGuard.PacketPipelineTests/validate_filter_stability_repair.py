#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

checks = {
    "thread-safe packet finalization": (
        ROOT / "SilkroadSecurityAPI/Packet.cs",
        ["lock (m_stateLock)", "CreateReadOnlyClone"]),
    "independent broadcast packets": (
        ROOT / "KMTGuard/ServerManagers/ServerManager.cs",
        ["packet.CreateReadOnlyClone()"]),
    "server direction budgets": (
        ROOT / "KMTGuard/Session/Session.cs",
        ["TryPassServerPacketGuards", "custom opcode rate"]),
    "database queue telemetry": (
        ROOT / "KMTGuard/Helpers/DatabaseJobQueue.cs",
        ["QueueHealth", "LastQueueAgeMs", "DroppedJobs"]),
    "database queue definitive outcomes": (
        ROOT / "KMTGuard/Helpers/DatabaseJobQueue.cs",
        ["CancelledBeforeStart", "TryCancelBeforeStart", "await completion.Task"]),
    "party packets independent from bookkeeping": (
        ROOT / "KMTGuard/Server/AgentServer/PacketHandler/Party/PartyData.cs",
        ["QueuePartyMemberDelete", "KeyedDatabaseJobQueue.TryQueueBackground", "SqlExecutionPolicy.BackgroundSeconds"]),
    "party bookkeeping preserves per-character order": (
        ROOT / "KMTGuard/Helpers/KeyedDatabaseJobQueue.cs",
        ["predecessor.WaitAsync", "RunContinuationsAsynchronously", "States.Remove(key)"]),
    "authoritative server packets bypass bookkeeping waits": (
        ROOT / "KMTGuard/Server/AgentServer/PacketHandler/COS/COSPackets.cs",
        ["await session.SendToClient(new Packet(packet))", "fellow skill persistence bookkeeping"]),
    "durable state follows persistence": (
        ROOT / "KMTGuard/Server/AgentServer/PacketHandler/UI/Title_IconManagers.cs",
        ["throw;", "targetCache[characterName] = ownedIcon.IconID"]),
    "runtime request authentication": (
        ROOT / "KMTGuard.RuntimeContract/RuntimeContracts.cs",
        ["RuntimeRequestAuthentication.Sign", "AuthenticationTag"]),
    "durable event rewards": (
        ROOT.parent.parent / "database/migrations/20260923_event_reward_outbox.sql",
        ["EventRewardOutbox", "UQ_EventRewardOutbox_Reward"]),
    "packaged event reward validation": (
        ROOT.parent.parent / "database/tests/event_reward_outbox_validation.sql",
        ["EventRewardOutbox", "UQ_EventRewardOutbox_Reward"]),
    "safe reward outbox transitions": (
        ROOT / "KMTGuard/Features/AutoEvents/AutoEventService.cs",
        ["TryTransitionRewardOutboxAsync", 'delivery.Succeeded ? "Completed" : "Pending"',
         "Keep a claimed row in Processing"]),
    "encrypted quick-login bridge": (
        ROOT / "KMTGuard/Server/QuickLoginAgentAuthBridge.cs",
        ["PasswordCipher", "AesGcm", "ZeroEncryptedPassword"]),
    "clearable gateway credential": (
        ROOT / "KMTGuard/Session/GatewayCredential.cs",
        ["CryptographicOperations.ZeroMemory", "ClearCore", "Dispose"]),
    "runtime health cancellation": (
        ROOT / "KMTGuard/Runtime/RuntimeControlServer.cs",
        ["HandleRequestAsync(request, requestLifetime.Token)",
         "SqlExecutionPolicy.RuntimeHealthSeconds", "cancellationToken: cancellationToken"]),
}

failed = []
for name, (path, needles) in checks.items():
    if not path.exists():
        failed.append(f"{name}: missing {path}")
        continue
    source = path.read_text(encoding="utf-8-sig")
    for needle in needles:
        if needle not in source:
            failed.append(f"{name}: missing {needle!r}")

version = (ROOT.parent.parent / "VERSION.txt").read_text(encoding="utf-8-sig").strip()
if version != "7.0.3":
    failed.append(f"release version mismatch: expected 7.0.3, found {version!r}")

changelog = (ROOT.parent.parent / "CHANGELOG.md").read_text(encoding="utf-8-sig")
if "## Update v7.0.3" not in changelog:
    failed.append("release changelog is missing v7.0.3")
if any(marker in changelog for marker in ("<<<<<<<", "=======", ">>>>>>>")):
    failed.append("release changelog still contains merge-conflict markers")

if failed:
    raise SystemExit("\n".join(failed))

print(f"Filter stability source checks passed ({len(checks)} groups).")
