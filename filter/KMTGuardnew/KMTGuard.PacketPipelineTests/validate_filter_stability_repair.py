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
    "runtime request authentication": (
        ROOT / "KMTGuard.RuntimeContract/RuntimeContracts.cs",
        ["RuntimeRequestAuthentication.Sign", "AuthenticationTag"]),
    "durable event rewards": (
        ROOT.parent.parent / "database/migrations/20260923_event_reward_outbox.sql",
        ["EventRewardOutbox", "UQ_EventRewardOutbox_Reward"]),
    "encrypted quick-login bridge": (
        ROOT / "KMTGuard/Server/QuickLoginAgentAuthBridge.cs",
        ["PasswordCipher", "AesGcm", "ZeroEncryptedPassword"]),
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

if failed:
    raise SystemExit("\n".join(failed))

print(f"Filter stability source checks passed ({len(checks)} groups).")
