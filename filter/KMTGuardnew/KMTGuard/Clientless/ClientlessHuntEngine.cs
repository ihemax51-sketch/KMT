using System.Collections.Concurrent;
using KMTGuard.ServerManagers;
using KMTGuard.SessionManager;
using Serilog;
using SilkroadSecurityAPI;

namespace KMTGuard.Clientless;

internal sealed class ClientlessHuntEngine
{
    private const int TickMilliseconds = 150;
    private const double MoveWhenFartherThanMeters = 2.5;
    private const double DefaultRunSpeedMetersPerSecond = 5.0;
    private static readonly ILogger HuntLog = Log.ForContext<ClientlessHuntEngine>();
    private static readonly ConcurrentDictionary<uint, int> NormalTargetOwners = new();

    private readonly ClientlessAccount _account;
    private readonly Func<Packet, CancellationToken, Task> _sendAsync;
    private readonly Dictionary<uint, HuntMonster> _monsters = new();
    private readonly Dictionary<uint, DateTime> _skillReadyAtUtc = new();
    private readonly HashSet<uint> _disabledAttackSkillIds = new();
    private readonly Dictionary<uint, int> _attackSkillFailureCounts = new();
    private readonly HashSet<uint> _activeBuffSkillIds = new();
    private readonly HashSet<uint> _disabledBuffSkillIds = new();
    private readonly Dictionary<uint, int> _buffFailureCounts = new();
    private readonly Dictionary<uint, uint> _buffSkillByToken = new();
    private readonly Dictionary<uint, DateTime> _buffConfirmationDeadlines = new();
    private readonly MemoryStream _groupSpawnBytes = new();
    private IReadOnlyList<ClientlessCombatSkill> _skills = Array.Empty<ClientlessCombatSkill>();
    private IReadOnlyList<ClientlessCombatSkill> _buffSkills = Array.Empty<ClientlessCombatSkill>();
    private IReadOnlyList<ClientlessPotionSlot> _potions = Array.Empty<ClientlessPotionSlot>();
    private ClientlessSpeedSlot? _speedSlot;
    private IReadOnlyList<ClientlessPetSlot> _petSlots = Array.Empty<ClientlessPetSlot>();
    private IReadOnlyList<ClientlessPetSupplySlot> _petSupplySlots = Array.Empty<ClientlessPetSupplySlot>();
    private IReadOnlySet<uint> _creatorFlagBuffSkillIds = new HashSet<uint>();
    private byte _groupSpawnType;
    private ushort _groupSpawnCount;
    private uint _selfUniqueId;
    private uint _targetUniqueId;
    private bool _targetSelected;
    private bool _dead;
    private bool _begun;
    private TeleportState _teleportState;
    private TeleportPurpose _teleportPurpose;
    private DateTime _teleportStateSinceUtc;
    private bool _returnAfterDeath;
    private bool _moveAwaitingResponse;
    private bool _movementInProgress;
    private bool _parkedInTown;
    private bool _hasConfirmedAttackSkill;
    private bool _hasConfirmedSelfBuff;
    private int _patrolIndex;
    private int _skillIndex;
    private int _buffIndex;
    private int _selectionAttempts;
    private float _currentX;
    private float _currentY;
    private float _currentZ;
    private int _currentRegion;
    private uint _currentHp;
    private uint _currentMp;
    private uint _maximumObservedHp;
    private uint _maximumObservedMp;
    private DateTime _targetStartedUtc;
    private DateTime _nextActionUtc;
    private DateTime _nextMoveUtc;
    private DateTime _nextPatrolUtc;
    private DateTime _nextHpPotionUtc;
    private DateTime _nextMpPotionUtc;
    private DateTime _nextBuffAttemptUtc;
    private DateTime _nextSpeedScrollUtc;
    private DateTime _speedScrollResponseDeadlineUtc;
    private bool _speedScrollPending;
    private DateTime _deadSinceUtc;
    private DateTime _lastPolicyCheckUtc;
    private DateTime _moveResponseDeadlineUtc;
    private DateTime _movementStartedUtc;
    private DateTime _movementArrivalUtc;
    private DateTime _movementStuckDeadlineUtc;
    private DateTime _nextSelectUtc;
    private DateTime _pendingActionSentUtc;
    private DateTime _nextTargetAcquireUtc;
    private DateTime _nextPetActionUtc;
    private DateTime _petSummonDeadlineUtc;
    private uint _attackPetUniqueId;
    private uint _grabPetUniqueId;
    private uint _attackPetTargetUniqueId;
    private uint _attackPetHp;
    private uint _attackPetMaxHp;
    private ushort _attackPetHunger;
    private bool _attackPetDead;
    private PetSummonKind _pendingPetSummon;
    private double _runSpeedMetersPerSecond = DefaultRunSpeedMetersPerSecond;
    private int _moveDestinationRegion;
    private float _moveDestinationX;
    private float _moveDestinationY;
    private float _moveDestinationZ;
    private int _moveSourceRegion;
    private float _moveSourceX;
    private float _moveSourceY;
    private float _moveSourceZ;
    private ClientlessHuntPolicy _policy = new();
    private ClientlessCombatSkill? _pendingSkill;
    private PendingCombatAction _pendingAction;
    private string _combatFeedback = string.Empty;

    public ClientlessHuntEngine(
        ClientlessAccount account,
        Func<Packet, CancellationToken, Task> sendAsync)
    {
        _account = account;
        _sendAsync = sendAsync;
        _currentRegion = account.HuntRegionID;
        _currentX = account.HuntX;
        _currentY = account.HuntY;
        _currentZ = account.HuntZ;
    }

    public static TimeSpan TickInterval => TimeSpan.FromMilliseconds(TickMilliseconds);

    public void Reset()
    {
        ReleaseTarget();
        _monsters.Clear();
        _groupSpawnBytes.SetLength(0);
        _groupSpawnType = 0;
        _groupSpawnCount = 0;
        _selfUniqueId = 0;
        _dead = false;
        _begun = false;
        _teleportState = TeleportState.Idle;
        _teleportPurpose = TeleportPurpose.None;
        _teleportStateSinceUtc = DateTime.MinValue;
        _returnAfterDeath = false;
        _moveAwaitingResponse = false;
        _movementInProgress = false;
        _parkedInTown = false;
        _hasConfirmedAttackSkill = false;
        _hasConfirmedSelfBuff = false;
        _skillReadyAtUtc.Clear();
        _disabledAttackSkillIds.Clear();
        _attackSkillFailureCounts.Clear();
        _activeBuffSkillIds.Clear();
        _disabledBuffSkillIds.Clear();
        _buffFailureCounts.Clear();
        _buffSkillByToken.Clear();
        _buffConfirmationDeadlines.Clear();
        _selectionAttempts = 0;
        _pendingAction = PendingCombatAction.None;
        _pendingSkill = null;
        _nextBuffAttemptUtc = DateTime.MinValue;
        _nextSpeedScrollUtc = DateTime.MinValue;
        _speedScrollResponseDeadlineUtc = DateTime.MinValue;
        _speedScrollPending = false;
        _petSlots = Array.Empty<ClientlessPetSlot>();
        _petSupplySlots = Array.Empty<ClientlessPetSupplySlot>();
        _attackPetUniqueId = 0;
        _grabPetUniqueId = 0;
        _attackPetTargetUniqueId = 0;
        _attackPetHp = 0;
        _attackPetMaxHp = 0;
        _attackPetHunger = 0;
        _attackPetDead = false;
        _pendingPetSummon = PetSummonKind.None;
        _nextTargetAcquireUtc = DateTime.MinValue;
        _nextPetActionUtc = DateTime.MinValue;
        _petSummonDeadlineUtc = DateTime.MinValue;
        _combatFeedback = string.Empty;
        _currentHp = 0;
        _currentMp = 0;
        _maximumObservedHp = 0;
        _maximumObservedMp = 0;
    }

    public void RefreshConfiguration(bool assignedAreaChanged)
    {
        _lastPolicyCheckUtc = DateTime.MinValue;

        if (!_account.HuntEnabled || assignedAreaChanged)
        {
            ReleaseTarget();
            _pendingAction = PendingCombatAction.None;
            _pendingSkill = null;
            _combatFeedback = string.Empty;
            _moveAwaitingResponse = false;
            _movementInProgress = false;
        }

        if (!assignedAreaChanged)
            return;

        _monsters.Clear();
        _moveAwaitingResponse = false;
        _movementInProgress = false;
        _teleportState = TeleportState.Idle;
        _teleportPurpose = TeleportPurpose.None;
        _teleportStateSinceUtc = DateTime.MinValue;
        _parkedInTown = false;
        _nextMoveUtc = DateTime.MinValue;
        _nextPatrolUtc = DateTime.MinValue;
    }

    public void CaptureCharacterData(Packet source)
    {
        var packet = Clone(source);
        try
        {
            // vSRO 0x34A5/0x3013/0x34A6 is one logical massive packet. The
            // session supplies the concatenated payload here; parsing an
            // arbitrary individual 0x3013 chunk yields random HP/MP values.
            if (packet.RemainingRead() < 59)
                return;

            packet.ReadUInt32(); // server timestamp (vSRO/Thailand+)
            packet.ReadUInt32();
            packet.ReadUInt8();
            packet.ReadUInt8();
            packet.ReadUInt8();
            packet.ReadUInt64();
            packet.ReadUInt32();
            packet.ReadUInt64();
            packet.ReadUInt32();
            packet.ReadUInt16();
            packet.ReadUInt8();
            packet.ReadUInt32();
            _currentHp = packet.ReadUInt32();
            _currentMp = packet.ReadUInt32();
            _maximumObservedHp = Math.Max(_maximumObservedHp, _currentHp);
            _maximumObservedMp = Math.Max(_maximumObservedMp, _currentMp);
        }
        catch (EndOfStreamException)
        {
            // Older protocol variants may have a shorter character header.
        }
    }

    public void BeginCharacterDataTransfer()
    {
        _monsters.Clear();
        _groupSpawnBytes.SetLength(0);
        _groupSpawnCount = 0;
        _groupSpawnType = 0;
        ReleaseTarget();
        _moveAwaitingResponse = false;
        _movementInProgress = false;
        _teleportState = TeleportState.Loading;
        _teleportStateSinceUtc = DateTime.UtcNow;
    }

    public void CompleteCharacterDataTransfer(Packet payload)
    {
        CaptureCharacterData(payload);
        if ((_teleportPurpose is TeleportPurpose.Town or TeleportPurpose.RespawnTown) && _currentHp > 0)
        {
            _dead = false;
            _returnAfterDeath = false;
        }
        _teleportState = TeleportState.Ready;
        _teleportStateSinceUtc = DateTime.UtcNow;
        if (_teleportPurpose == TeleportPurpose.Town)
            _parkedInTown = true;
        _moveAwaitingResponse = false;
        _movementInProgress = false;
        _nextPatrolUtc = DateTime.UtcNow.AddSeconds(1);
    }

    public async Task BeginAsync(CancellationToken cancellationToken)
    {
        if (_begun)
            return;

        _begun = true;
        _policy = await ClientlessManager.GetHuntPolicyAsync(force: true);
        _skills = await ClientlessManager.LoadCombatSkillsAsync(_account.CharacterName);
        _buffSkills = await ClientlessManager.LoadSelfBuffSkillsAsync(_account.CharacterName);
        _potions = await ClientlessManager.LoadPotionSlotsAsync(_account.CharacterName);
        _speedSlot = await ClientlessManager.LoadSpeedSlotAsync(_account.CharacterName);
        _petSlots = await ClientlessManager.LoadPetSlotsAsync(_account.CharacterName);
        _petSupplySlots = await ClientlessManager.LoadPetSupplySlotsAsync(_account.CharacterName);
        _creatorFlagBuffSkillIds = await ClientlessManager.LoadCreatorFlagBuffSkillIdsAsync();
        HuntLog.Information(
            "Clientless {Character} combat profile is ready: {AttackSkillCount} attack skill(s), {BuffSkillCount} self buff(s), {PotionCount} potion slot(s), speed scroll={HasSpeedScroll}, attack pet={HasAttackPet}, grab pet={HasGrabPet}.",
            _account.CharacterName,
            _skills.Count,
            _buffSkills.Count,
            _potions.Count,
            _speedSlot != null,
            _petSlots.Any(item => item.PetKind == 1),
            _petSlots.Any(item => item.PetKind == 2));
        if (_policy.UseSkills && _skills.Count == 0)
        {
            HuntLog.Error(
                "Clientless {Character} has no valid learned monster-attack skill for its equipped weapon. Run Prepare Existing Combat; this profile will be reported explicitly instead of being treated as a successful skill build.",
                _account.CharacterName);
        }

        if (!_account.HuntEnabled)
        {
            await ReturnToTownAsync(cancellationToken, "Hunting is paused; standing in town");
            return;
        }

        if (!_account.HuntAreaID.HasValue || _account.HuntRegionID <= 0)
        {
            await SetStatusAsync("NeedsArea", "No enabled hunting area exists for this city.");
            return;
        }

        if (!_policy.Enabled)
        {
            await ReturnToTownAsync(cancellationToken, "Hunting is paused globally; standing in town");
            return;
        }

        await MoveToAssignedAreaAsync(cancellationToken, "Moving to assigned hunting area");
    }

    public async Task HandlePacketAsync(Packet source, CancellationToken cancellationToken)
    {
        var packet = Clone(source);
        try
        {
            switch (packet.Opcode)
            {
                case 0x3013:
                    // Character-data chunks are accumulated by ClientlessSession
                    // between 0x34A5 and 0x34A6 and parsed exactly once.
                    break;
                case 0x3020:
                    _selfUniqueId = packet.ReadUInt32();
                    break;
                case 0x3015:
                    TryParseMonster(packet, isGroup: false);
                    break;
                case 0x3016:
                    RemoveMonster(packet.ReadUInt32());
                    break;
                case 0x3017:
                    _groupSpawnType = packet.ReadUInt8();
                    _groupSpawnCount = packet.ReadUInt16();
                    _groupSpawnBytes.SetLength(0);
                    break;
                case 0x3019:
                    var bytes = packet.GetBytes();
                    _groupSpawnBytes.Write(bytes, 0, bytes.Length);
                    break;
                case 0x3018:
                    ParseGroupSpawn();
                    break;
                case 0x30BF:
                    HandleLifeState(packet);
                    break;
                case 0x30C8:
                    HandleCosData(packet);
                    break;
                case 0x30C9:
                    HandleCosUpdate(packet);
                    break;
                case 0x3057:
                    HandleHealthUpdate(packet);
                    break;
                case 0x303D:
                    HandleStatsUpdate(packet);
                    break;
                case 0xB021:
                    HandleMovement(packet);
                    break;
                case 0xB023:
                    HandleAuthoritativePosition(packet, uniqueIdFirst: true, stopsMovement: true);
                    break;
                case 0x3028:
                    HandleAuthoritativePosition(packet, uniqueIdFirst: false, stopsMovement: false);
                    break;
                case 0x30D0:
                    HandleMoveSpeed(packet);
                    break;
                case 0xB045:
                    HandleSelectionResponse(packet);
                    break;
                case 0xB070:
                    HandleSkillCastResponse(packet);
                    break;
                case 0xB0BD:
                    HandleBuffApplied(packet);
                    break;
                case 0xB072:
                    HandleBuffRemoved(packet);
                    break;
                case 0xB04C:
                    HandleItemUseResponse(packet);
                    break;
            }
        }
        catch (Exception ex) when (ex is EndOfStreamException or InvalidDataException or ArgumentOutOfRangeException)
        {
            HuntLog.Verbose("Clientless hunt ignored incompatible 0x{Opcode:X4} payload for {Character}: {Reason}",
                source.Opcode, _account.CharacterName, ex.Message);
        }

        await Task.CompletedTask;
    }

    public async Task TickAsync(CancellationToken cancellationToken)
    {
        if (!_begun)
            return;

        var now = DateTime.UtcNow;
        if (now - _lastPolicyCheckUtc >= TimeSpan.FromSeconds(3))
        {
            _policy = await ClientlessManager.GetHuntPolicyAsync();
            _lastPolicyCheckUtc = now;
        }

        if (!_account.HuntEnabled || !_policy.Enabled)
        {
            ReleaseTarget();
            _moveAwaitingResponse = false;
            _movementInProgress = false;
            await ReturnToTownAsync(
                cancellationToken,
                !_account.HuntEnabled
                    ? "Hunting is paused; standing in town"
                    : "Hunting is paused globally; standing in town");
            return;
        }

        _parkedInTown = false;

        if (!_account.HuntAreaID.HasValue || _account.HuntRegionID <= 0)
        {
            ReleaseTarget();
            await SetStatusAsync("NeedsArea", "No enabled hunting area exists for this city.");
            return;
        }

        if (_dead)
        {
            ReleaseTarget();
            await SetStatusAsync("Recovering", "Character is dead; returning to town.");
            if (now - _deadSinceUtc >= TimeSpan.FromSeconds(5) &&
                (!_returnAfterDeath || now >= _nextMoveUtc))
            {
                await _sendAsync(ClientlessHuntProtocol.BuildRespawnInTown(), cancellationToken);
                _returnAfterDeath = true;
                _teleportPurpose = TeleportPurpose.RespawnTown;
                _teleportState = TeleportState.RequestSent;
                _teleportStateSinceUtc = now;
                _nextMoveUtc = now.AddSeconds(15);
            }
            return;
        }

        if (_returnAfterDeath && now >= _nextMoveUtc)
        {
            // Life-state/character-data confirmation clears the death flag.
            // Never issue an area teleport merely because a fixed delay elapsed.
            _nextMoveUtc = now.AddSeconds(15);
            return;
        }

        if (_teleportState == TeleportState.Ready && _teleportPurpose != TeleportPurpose.HuntArea)
        {
            _teleportState = TeleportState.Idle;
            _teleportStateSinceUtc = now;
        }

        if (_teleportState == TeleportState.Idle)
        {
            await MoveToAssignedAreaAsync(cancellationToken, "Moving to assigned hunting area");
            return;
        }

        if (_teleportState is TeleportState.RequestSent or TeleportState.Loading)
        {
            var timeout = _teleportState == TeleportState.RequestSent
                ? TimeSpan.FromSeconds(15)
                : TimeSpan.FromSeconds(30);
            if (now - _teleportStateSinceUtc >= timeout)
            {
                _teleportState = TeleportState.Idle;
                _teleportStateSinceUtc = now;
                await SetStatusAsync("Positioning", "Teleport did not complete; retrying safely");
            }
            else
            {
                await SetStatusAsync("Positioning", "Waiting for the GameServer teleport loading cycle");
            }
            return;
        }

        if (_moveAwaitingResponse && now >= _moveResponseDeadlineUtc)
        {
            _moveAwaitingResponse = false;
            _movementInProgress = false;
            _nextMoveUtc = now.AddMilliseconds(750);
            HuntLog.Warning(
                "Clientless {Character} did not receive its vSRO B021 movement acknowledgement; the position was not advanced and movement will be retried.",
                _account.CharacterName);
            await SetStatusAsync(
                _targetUniqueId == 0 ? "Patrolling" : "Fighting",
                _targetUniqueId == 0
                    ? "Movement was not acknowledged; retrying patrol"
                    : "Movement was not acknowledged; retrying approach");
            return;
        }

        if (_moveAwaitingResponse)
        {
            await SetStatusAsync(
                _targetUniqueId == 0 ? "Positioning" : "Fighting",
                _targetUniqueId == 0 ? "Waiting for movement confirmation" : "Approaching selected target");
            return;
        }

        if (_movementInProgress)
        {
            if (now >= _movementArrivalUtc)
            {
                CompleteEstimatedMovement();
                _nextMoveUtc = now.AddMilliseconds(250);
            }
            else if (now >= _movementStuckDeadlineUtc)
            {
                _movementInProgress = false;
                _nextMoveUtc = now.AddMilliseconds(750);
                ReleaseTarget();
                await SetStatusAsync("Patrolling", "Movement stalled; selecting a fresh route and target");
                return;
            }
            else
            {
                UpdateEstimatedMovement(now);
                await SetStatusAsync(
                    _targetUniqueId == 0 ? "Positioning" : "Fighting",
                    _targetUniqueId == 0 ? "Patrolling inside the hunting area" : "Approaching selected target");
                return;
            }
        }

        if (_pendingAction != PendingCombatAction.None)
        {
            if (now - _pendingActionSentUtc < PendingActionFeedbackTimeout())
            {
                var pendingTarget = _targetUniqueId != 0 && _monsters.TryGetValue(_targetUniqueId, out var pendingMonster)
                    ? pendingMonster.Name
                    : string.Empty;
                await SetStatusAsync(
                    _pendingAction == PendingCombatAction.Buff ? "Buffing" : "Fighting",
                    PendingActionDescription(pendingTarget),
                    string.IsNullOrWhiteSpace(pendingTarget) ? null : pendingTarget);
                return;
            }

            RegisterPendingActionFailure(
                errorCode: 0,
                reason: "no matching vSRO B070 response",
                now);
        }

        ExpireUnconfirmedBuffs(now);

        if (_speedScrollPending && now >= _speedScrollResponseDeadlineUtc)
        {
            _speedScrollPending = false;
            _nextSpeedScrollUtc = now.AddSeconds(20);
        }

        if (await TryUsePotionAsync(now, cancellationToken))
            return;

        if (await TryMaintainPetsAsync(now, cancellationToken))
            return;

        if (_targetUniqueId != 0 && now - _targetStartedUtc > TimeSpan.FromSeconds(_policy.TargetTimeoutSeconds))
        {
            ReleaseTarget();
            _nextPatrolUtc = now;
            await SetStatusAsync("Patrolling", "Target timed out; selecting another monster without teleporting.");
            return;
        }

        if (_targetUniqueId != 0 &&
            (!_monsters.TryGetValue(_targetUniqueId, out var selected) || !IsEligible(selected)))
        {
            ReleaseTarget();
        }

        if (_targetUniqueId == 0 && now >= _nextTargetAcquireUtc)
            AcquireTarget(now);

        if (_targetUniqueId == 0)
        {
            if (await TryUseSpeedScrollAsync(now, cancellationToken))
                return;
            if (_policy.UseSkills && await TryUseBuffAsync(now, cancellationToken))
                return;

            await SetStatusAsync("Patrolling", $"{_account.HuntAreaName}: searching for monsters");
            if (now >= _nextPatrolUtc && !_moveAwaitingResponse)
            {
                await PatrolAsync(cancellationToken);
                _nextPatrolUtc = now.AddSeconds(4);
            }
            return;
        }

        var target = _monsters[_targetUniqueId];
        var distance = DistanceMeters(_currentRegion, _currentX, _currentZ, target.RegionID, target.X, target.Z);
        var desiredRange = GetDesiredCombatRangeMeters(now);
        if (distance > desiredRange && now >= _nextMoveUtc && !_moveAwaitingResponse)
        {
            await SetStatusAsync("Fighting", $"Approaching: {target.Name}", target.Name);
            await MoveTowardAsync(target, cancellationToken);
            _nextMoveUtc = now.AddMilliseconds(900);
            return;
        }

        if (!_targetSelected)
        {
            if (now >= _nextSelectUtc)
            {
                _selectionAttempts++;
                await _sendAsync(ClientlessHuntProtocol.BuildSelectTarget(target.UniqueId), cancellationToken);
                _nextSelectUtc = now.AddMilliseconds(800);
                HuntLog.Debug(
                    "Clientless {Character} selecting target {Target} ({TargetId}), attempt {Attempt}.",
                    _account.CharacterName,
                    target.Name,
                    target.UniqueId,
                    _selectionAttempts);
            }
            await SetStatusAsync("Fighting", $"Selecting target: {target.Name}", target.Name);
            return;
        }

        await CommandAttackPetAsync(target.UniqueId, cancellationToken);

        if (await TryUseSpeedScrollAsync(now, cancellationToken))
            return;

        if (_policy.UseSkills && await TryUseBuffAsync(now, cancellationToken))
            return;

        if (now < _nextActionUtc)
        {
            await SetStatusAsync("Fighting", FightDescription(target.Name), target.Name);
            return;
        }

        if (_policy.UseSkills && await TryUseSkillAsync(target, distance, now, cancellationToken))
            return;

        if (_policy.UseSkills && _skills.Count == 0)
        {
            await SetStatusAsync(
                "CombatBlocked",
                "No valid learned attack skill matches the equipped weapon. Run Prepare Existing Combat.",
                target.Name);
            return;
        }

        if (_policy.UseBasicAttack)
        {
            await _sendAsync(ClientlessHuntProtocol.BuildBasicAttack(target.UniqueId), cancellationToken);
            BeginPendingAction(PendingCombatAction.BasicAttack, now);
            _combatFeedback = $"Basic attack fallback: {target.Name}";
            _nextActionUtc = now.AddMilliseconds(1300);
            await SetStatusAsync("Fighting", _combatFeedback, target.Name);
            return;
        }

        await SetStatusAsync(
            "CombatBlocked",
            _skills.Count == 0
                ? "No learned monster-attack skill matches the equipped weapon."
                : "No learned attack skill is currently usable.",
            target.Name);
    }

    private async Task MoveToAssignedAreaAsync(CancellationToken cancellationToken, string message)
    {
        var proxySession = ServerManager.AgentSessions.FindByCharName(_account.CharacterName);
        if (proxySession == null || !proxySession.CharacterGameReady)
        {
            await SetStatusAsync("Waiting", "Waiting for the character to finish entering the game.");
            return;
        }

        _monsters.Clear();
        ReleaseTarget();
        await TeleportFreezeService.TeleportToPositionAsync(
            proxySession,
            1,
            _account.HuntRegionID,
            Convert.ToInt32(MathF.Round(_account.HuntX)),
            Convert.ToInt32(MathF.Round(_account.HuntY)),
            Convert.ToInt32(MathF.Round(_account.HuntZ)));
        _moveAwaitingResponse = false;
        _movementInProgress = false;
        // 0x34A5..0x34A6 character data is the readiness barrier. Do not
        // patrol/select/cast merely because the custom teleport request was
        // accepted by the Filter thread.
        _teleportState = TeleportState.RequestSent;
        _teleportPurpose = TeleportPurpose.HuntArea;
        _teleportStateSinceUtc = DateTime.UtcNow;
        _parkedInTown = false;
        _nextPatrolUtc = DateTime.UtcNow.AddSeconds(2);
        await SetStatusAsync("Positioning", $"{message}: {_account.HuntAreaName}");
    }

    private async Task ReturnToTownAsync(CancellationToken cancellationToken, string message)
    {
        if (_parkedInTown)
        {
            await SetStatusAsync("Parked", message);
            return;
        }

        if (_teleportState is TeleportState.RequestSent or TeleportState.Loading)
        {
            var now = DateTime.UtcNow;
            var timeout = _teleportState == TeleportState.RequestSent
                ? TimeSpan.FromSeconds(15)
                : TimeSpan.FromSeconds(30);
            if (now - _teleportStateSinceUtc < timeout)
            {
                await SetStatusAsync(
                    "Positioning",
                    _teleportPurpose is TeleportPurpose.Town or TeleportPurpose.RespawnTown
                        ? "Returning to town; waiting for the GameServer loading cycle"
                        : "Finishing the current loading cycle before returning to town");
                return;
            }

            _teleportState = TeleportState.Idle;
            _teleportStateSinceUtc = now;
        }

        _monsters.Clear();
        ReleaseTarget();
        _moveAwaitingResponse = false;
        _movementInProgress = false;

        if (_dead)
        {
            await _sendAsync(ClientlessHuntProtocol.BuildRespawnInTown(), cancellationToken);
            _returnAfterDeath = true;
            _teleportState = TeleportState.RequestSent;
            _teleportPurpose = TeleportPurpose.RespawnTown;
            _teleportStateSinceUtc = DateTime.UtcNow;
            _nextMoveUtc = DateTime.UtcNow.AddSeconds(15);
            _parkedInTown = false;
            await SetStatusAsync("Positioning", "Respawning in town; waiting for the GameServer loading cycle");
            return;
        }

        var proxySession = ServerManager.AgentSessions.FindByCharName(_account.CharacterName);
        if (proxySession == null || !proxySession.CharacterGameReady)
        {
            await SetStatusAsync("Waiting", "Waiting for the character to finish entering the game before returning to town.");
            return;
        }

        var town = _account.TownParkingRegionID > 0
            ? new ClientlessTownPositions.Position(
                _account.TownParkingRegionID,
                _account.TownParkingX,
                _account.TownParkingY,
                _account.TownParkingZ)
            : ClientlessTownPositions.Resolve(_account.City, _account.ID);
        await TeleportFreezeService.TeleportToPositionAsync(
            proxySession,
            1,
            town.RegionID,
            Convert.ToInt32(MathF.Round(town.X)),
            Convert.ToInt32(MathF.Round(town.Y)),
            Convert.ToInt32(MathF.Round(town.Z)));
        _teleportState = TeleportState.RequestSent;
        _teleportPurpose = TeleportPurpose.Town;
        _teleportStateSinceUtc = DateTime.UtcNow;
        _parkedInTown = false;
        await SetStatusAsync("Positioning", "Returning to town; waiting for the GameServer loading cycle");
    }

    private void AcquireTarget(DateTime now)
    {
        var candidates = _monsters.Values
            .Where(IsEligible)
            .OrderByDescending(monster => _policy.UniquePriority && monster.IsUnique)
            .ThenBy(monster => DistanceMeters(_currentRegion, _currentX, _currentZ, monster.RegionID, monster.X, monster.Z));

        foreach (var candidate in candidates)
        {
            if (!candidate.IsUnique && !NormalTargetOwners.TryAdd(candidate.UniqueId, _account.ID))
                continue;

            _targetUniqueId = candidate.UniqueId;
            _targetSelected = false;
            _targetStartedUtc = now;
            _nextActionUtc = now;
            _nextSelectUtc = now;
            _nextTargetAcquireUtc = DateTime.MinValue;
            _selectionAttempts = 0;
            _combatFeedback = string.Empty;
            return;
        }
    }

    private bool IsEligible(HuntMonster monster)
    {
        if (!monster.Alive || (monster.IsUnique ? !_policy.AttackUnique : !_policy.AttackNormal))
            return false;
        var centerDistance = DistanceMeters(
            _account.HuntRegionID, _account.HuntX, _account.HuntZ,
            monster.RegionID, monster.X, monster.Z);
        return centerDistance <= Math.Clamp(_account.HuntRadius, 5, 500);
    }

    private async Task MoveTowardAsync(HuntMonster target, CancellationToken cancellationToken)
    {
        var desiredRange = GetDesiredCombatRangeMeters(DateTime.UtcNow);
        var currentWorld = ToWorldRaw(_currentRegion, _currentX, _currentZ);
        var targetWorld = ToWorldRaw(target.RegionID, target.X, target.Z);
        var dx = targetWorld.X - currentWorld.X;
        var dz = targetWorld.Z - currentWorld.Z;
        var rawDistance = Math.Sqrt((dx * dx) + (dz * dz));
        if (rawDistance <= 0.01)
            return;

        // Stop just inside the usable range. Moving to the target's exact
        // center made Wizard/Bow characters behave like melee characters.
        var keepRaw = Math.Max(
            MoveWhenFartherThanMeters * 10d,
            (desiredRange - 0.75d) * 10d);
        var travelRaw = Math.Max(0d, rawDistance - keepRaw);
        var ratio = travelRaw / rawDistance;
        var approach = FromWorldRaw(
            (float)(currentWorld.X + (dx * ratio)),
            (float)(currentWorld.Z + (dz * ratio)),
            target.Y);
        await SendMoveAsync(approach.Region, approach.X, approach.Y, approach.Z, cancellationToken);
    }

    private async Task PatrolAsync(CancellationToken cancellationToken)
    {
        var radiusRaw = Math.Clamp(_account.HuntRadius, 5, 500) * 6.5f;
        var angle = (_patrolIndex++ % 8) * (MathF.PI / 4f);
        var x = _account.HuntX + (MathF.Cos(angle) * radiusRaw);
        var z = _account.HuntZ + (MathF.Sin(angle) * radiusRaw);
        var normalized = NormalizeFieldPosition(_account.HuntRegionID, x, _account.HuntY, z);
        await SendMoveAsync(normalized.Region, normalized.X, normalized.Y, normalized.Z, cancellationToken);
    }

    private async Task SendMoveAsync(int regionId, float x, float y, float z, CancellationToken cancellationToken)
    {
        var now = DateTime.UtcNow;
        var distance = DistanceMeters(_currentRegion, _currentX, _currentZ, regionId, x, z);
        await _sendAsync(ClientlessHuntProtocol.BuildMove(regionId, x, y, z), cancellationToken);
        _moveSourceRegion = _currentRegion;
        _moveSourceX = _currentX;
        _moveSourceY = _currentY;
        _moveSourceZ = _currentZ;
        _moveDestinationRegion = regionId;
        _moveDestinationX = x;
        _moveDestinationY = y;
        _moveDestinationZ = z;
        _moveAwaitingResponse = true;
        _movementInProgress = false;
        _movementStartedUtc = now;
        _movementArrivalUtc = now.Add(EstimateMovementDuration(distance, _runSpeedMetersPerSecond));
        _moveResponseDeadlineUtc = now.AddSeconds(3);
        _movementStuckDeadlineUtc = _movementArrivalUtc.AddSeconds(
            Math.Clamp(_policy.StuckSeconds, (short)5, (short)120));
    }

    private double GetDesiredCombatRangeMeters(DateTime now)
    {
        if (!_policy.UseSkills)
            return MoveWhenFartherThanMeters;

        var readyRange = _skills
            .Where(skill => !_disabledAttackSkillIds.Contains(skill.ID))
            .Where(skill => !_skillReadyAtUtc.TryGetValue(skill.ID, out var readyAt) || now >= readyAt)
            .Where(skill => skill.ActionRange > 0)
            .Select(skill => skill.ActionRange / 10d)
            .DefaultIfEmpty(MoveWhenFartherThanMeters)
            .Max();
        return Math.Clamp(readyRange - 0.5d, MoveWhenFartherThanMeters, 40d);
    }

    private async Task<bool> TryUseSkillAsync(
        HuntMonster target,
        double distance,
        DateTime now,
        CancellationToken cancellationToken)
    {
        if (_skills.Count == 0)
            return false;

        for (var attempt = 0; attempt < _skills.Count; attempt++)
        {
            var skill = _skills[_skillIndex++ % _skills.Count];
            if (_disabledAttackSkillIds.Contains(skill.ID))
                continue;
            if (_skillReadyAtUtc.TryGetValue(skill.ID, out var readyAt) && now < readyAt)
                continue;
            if (skill.ActionRange > 0 && distance > (skill.ActionRange / 10d) + 1d)
                continue;

            var mpPreparation = await PrepareMpForSkillAsync(skill, now, cancellationToken, target.Name);
            if (mpPreparation != MpPreparationResult.Ready)
                return mpPreparation == MpPreparationResult.PotionSent;

            BeginPendingAction(PendingCombatAction.Skill, now, skill);
            await _sendAsync(ClientlessHuntProtocol.BuildSkillAttack(skill.ID, target.UniqueId), cancellationToken);
            _combatFeedback = $"Using learned skill against {target.Name}";
            _nextActionUtc = now.AddMilliseconds(700);
            await SetStatusAsync("Fighting", _combatFeedback, target.Name);
            return true;
        }

        return false;
    }

    private async Task<bool> TryUseBuffAsync(DateTime now, CancellationToken cancellationToken)
    {
        if (_buffSkills.Count == 0 || now < _nextActionUtc || now < _nextBuffAttemptUtc)
            return false;

        for (var attempt = 0; attempt < _buffSkills.Count; attempt++)
        {
            var skill = _buffSkills[_buffIndex++ % _buffSkills.Count];
            if (_activeBuffSkillIds.Contains(skill.ID) ||
                _buffConfirmationDeadlines.ContainsKey(skill.ID) ||
                _disabledBuffSkillIds.Contains(skill.ID))
                continue;
            if (_skillReadyAtUtc.TryGetValue(skill.ID, out var readyAt) && now < readyAt)
                continue;

            var mpPreparation = await PrepareMpForSkillAsync(skill, now, cancellationToken, null);
            if (mpPreparation != MpPreparationResult.Ready)
                return mpPreparation == MpPreparationResult.PotionSent;

            BeginPendingAction(PendingCombatAction.Buff, now, skill);
            await _sendAsync(ClientlessHuntProtocol.BuildSelfBuff(skill.ID, _selfUniqueId, skill.TargetsSelf), cancellationToken);
            _combatFeedback = $"Casting self buff: {skill.CodeName128}";
            _nextBuffAttemptUtc = now.AddSeconds(2);
            _nextActionUtc = now.AddMilliseconds(700);
            await SetStatusAsync("Buffing", _combatFeedback);
            return true;
        }

        return false;
    }

    private async Task<MpPreparationResult> PrepareMpForSkillAsync(
        ClientlessCombatSkill skill,
        DateTime now,
        CancellationToken cancellationToken,
        string? targetName)
    {
        if (skill.ConsumeMp <= 0 || _currentMp >= skill.ConsumeMp)
            return MpPreparationResult.Ready;
        if (_currentMp == 0 || now < _nextMpPotionUtc)
            return MpPreparationResult.Blocked;

        var potion = _potions.FirstOrDefault(item => item.TypeID4 == 2);
        if (potion == null)
            return MpPreparationResult.Blocked;

        await _sendAsync(ClientlessHuntProtocol.BuildUseItem(potion.Slot, potion.TypeID), cancellationToken);
        _nextMpPotionUtc = now.AddMilliseconds(Math.Clamp(potion.CooldownMs, 1000, 30000));
        _nextActionUtc = now.AddMilliseconds(900);
        _combatFeedback = $"Restoring MP before {skill.CodeName128}";
        await SetStatusAsync("Recovering", _combatFeedback, targetName);
        return MpPreparationResult.PotionSent;
    }

    private async Task<bool> TryUsePotionAsync(DateTime now, CancellationToken cancellationToken)
    {
        if (_potions.Count == 0)
            return false;

        ClientlessPotionSlot? potion = null;
        var isHpPotion = false;
        if (now >= _nextHpPotionUtc &&
            _maximumObservedHp > 0 &&
            _currentHp * 100UL <= _maximumObservedHp * _policy.HpPotionPercent)
        {
            potion = _potions.FirstOrDefault(item => item.TypeID4 == 1);
            isHpPotion = potion != null;
        }
        else if (now >= _nextMpPotionUtc &&
                 _maximumObservedMp > 0 &&
                 _currentMp * 100UL <= _maximumObservedMp * _policy.MpPotionPercent)
            potion = _potions.FirstOrDefault(item => item.TypeID4 == 2);

        if (potion == null)
            return false;

        await _sendAsync(ClientlessHuntProtocol.BuildUseItem(potion.Slot, potion.TypeID), cancellationToken);
        if (isHpPotion)
            _nextHpPotionUtc = now.AddMilliseconds(Math.Clamp(potion.CooldownMs, 1000, 30000));
        else
            _nextMpPotionUtc = now.AddMilliseconds(Math.Clamp(potion.CooldownMs, 1000, 30000));
        return true;
    }

    private async Task<bool> TryMaintainPetsAsync(DateTime now, CancellationToken cancellationToken)
    {
        if (_petSlots.Count == 0 || now < _nextPetActionUtc)
            return false;

        if (_pendingPetSummon != PetSummonKind.None)
        {
            if (now < _petSummonDeadlineUtc)
                return false;

            HuntLog.Warning(
                "Clientless {Character} did not receive 0x30C8 confirmation for its {PetKind} pet; summon will be retried safely.",
                _account.CharacterName,
                _pendingPetSummon);
            _pendingPetSummon = PetSummonKind.None;
            _nextPetActionUtc = now.AddSeconds(3);
            return false;
        }

        var attackPet = _petSlots.FirstOrDefault(item => item.PetKind == 1);
        if (_attackPetUniqueId == 0 && attackPet != null)
        {
            if (_attackPetDead)
            {
                var revival = _petSupplySlots.FirstOrDefault(item => item.SupplyKind == 2);
                if (revival != null)
                {
                    await _sendAsync(
                        ClientlessHuntProtocol.BuildUseItemForSlot(revival.Slot, revival.TypeID, attackPet.Slot),
                        cancellationToken);
                    _attackPetDead = false;
                    _nextPetActionUtc = now.AddSeconds(5);
                    HuntLog.Information(
                        "Clientless {Character} is reviving its attack pet from inventory slot {PetSlot}.",
                        _account.CharacterName,
                        attackPet.Slot);
                    return true;
                }
            }

            await _sendAsync(ClientlessHuntProtocol.BuildUseItem(attackPet.Slot, attackPet.TypeID), cancellationToken);
            _pendingPetSummon = PetSummonKind.Attack;
            _petSummonDeadlineUtc = now.AddSeconds(4);
            _nextPetActionUtc = now.AddMilliseconds(750);
            return true;
        }

        var grabPet = _petSlots.FirstOrDefault(item => item.PetKind == 2);
        if (_grabPetUniqueId == 0 && grabPet != null)
        {
            await _sendAsync(ClientlessHuntProtocol.BuildUseItem(grabPet.Slot, grabPet.TypeID), cancellationToken);
            _pendingPetSummon = PetSummonKind.Grab;
            _petSummonDeadlineUtc = now.AddSeconds(4);
            _nextPetActionUtc = now.AddMilliseconds(750);
            return true;
        }

        if (_attackPetUniqueId != 0 && _attackPetMaxHp > 0 &&
            _attackPetHp * 100UL <= _attackPetMaxHp * 70UL)
        {
            var healthPotion = _petSupplySlots.FirstOrDefault(item => item.SupplyKind == 1);
            if (healthPotion != null)
            {
                await _sendAsync(
                    ClientlessHuntProtocol.BuildUseItemFor(healthPotion.Slot, healthPotion.TypeID, _attackPetUniqueId),
                    cancellationToken);
                _nextPetActionUtc = now.AddMilliseconds(1200);
                return true;
            }
        }

        if (_attackPetUniqueId != 0 && _attackPetHunger > 0 && _attackPetHunger <= 1000)
        {
            var hungerPotion = _petSupplySlots.FirstOrDefault(item => item.SupplyKind == 3);
            if (hungerPotion != null)
            {
                await _sendAsync(
                    ClientlessHuntProtocol.BuildUseItemFor(hungerPotion.Slot, hungerPotion.TypeID, _attackPetUniqueId),
                    cancellationToken);
                _nextPetActionUtc = now.AddMilliseconds(1200);
                return true;
            }
        }

        _nextPetActionUtc = now.AddMilliseconds(500);
        return false;
    }

    private async Task CommandAttackPetAsync(uint targetUniqueId, CancellationToken cancellationToken)
    {
        if (_attackPetUniqueId == 0 || _attackPetDead || _attackPetTargetUniqueId == targetUniqueId)
            return;

        await _sendAsync(
            ClientlessHuntProtocol.BuildCosAttack(_attackPetUniqueId, targetUniqueId),
            cancellationToken);
        _attackPetTargetUniqueId = targetUniqueId;
    }

    private async Task<bool> TryUseSpeedScrollAsync(DateTime now, CancellationToken cancellationToken)
    {
        if (_speedSlot == null || _speedScrollPending || now < _nextSpeedScrollUtc)
            return false;

        await _sendAsync(
            ClientlessHuntProtocol.BuildUseItem(_speedSlot.Slot, _speedSlot.TypeID),
            cancellationToken);
        _speedScrollPending = true;
        _speedScrollResponseDeadlineUtc = now.AddSeconds(3);
        _nextSpeedScrollUtc = now.AddSeconds(20);
        await SetStatusAsync("Buffing", "Using automatic speed scroll");
        return true;
    }

    private void HandleItemUseResponse(Packet packet)
    {
        var result = packet.ReadUInt8();
        byte? sourceSlot = null;
        if (result == 1 && packet.RemainingRead() >= 1)
            sourceSlot = packet.ReadUInt8();

        if (_pendingPetSummon != PetSummonKind.None && sourceSlot.HasValue)
        {
            var expectedKind = _pendingPetSummon == PetSummonKind.Attack ? (byte)1 : (byte)2;
            var expected = _petSlots.FirstOrDefault(item => item.PetKind == expectedKind);
            if (expected != null && sourceSlot.Value == expected.Slot)
            {
                // 0xB04C only acknowledges use of the scroll. 0x30C8 remains
                // authoritative for the actual COS unique ID and active state.
                _petSummonDeadlineUtc = DateTime.UtcNow.AddSeconds(4);
                return;
            }
        }

        if (!_speedScrollPending || _speedSlot == null)
            return;

        if (result != 1)
        {
            _speedScrollPending = false;
            _nextSpeedScrollUtc = DateTime.UtcNow.AddSeconds(30);
            return;
        }

        if (!sourceSlot.HasValue || sourceSlot.Value != _speedSlot.Slot)
            return;

        if (packet.GetBytes().Length >= 4)
            packet.ReadUInt16(); // remaining stack

        _speedScrollPending = false;
        // Standard vSRO speed drugs last 30 minutes. Renew just after expiry;
        // if a server customizes the duration, an early request is rejected and
        // the normal retry path safely tries again without interrupting combat.
        _nextSpeedScrollUtc = DateTime.UtcNow.AddMinutes(30).AddSeconds(2);
        HuntLog.Information(
            "Clientless {Character} activated speed scroll {CodeName} from slot {Slot}; renewal is scheduled.",
            _account.CharacterName,
            _speedSlot.CodeName128,
            _speedSlot.Slot);
    }

    private void ParseGroupSpawn()
    {
        if (_groupSpawnCount == 0)
            return;

        var group = new Packet(0x3019, false, false, _groupSpawnBytes.ToArray());
        group.ToReadOnly();
        try
        {
            if (_groupSpawnType == 1)
            {
                // 0x3019 contains exactly Count complete heterogeneous entities.
                // Never search byte-by-byte for a monster RefID: player names,
                // inventory items and drop payloads can contain the same four
                // bytes and create a phantom UID/position. Consume every entity
                // from its real boundary, even when it is irrelevant to hunting.
                for (var index = 0; index < _groupSpawnCount; index++)
                {
                    if (!TryParseGroupSpawnEntity(group))
                        throw new InvalidDataException($"Unsupported or incomplete group-spawn entity at index {index}.");
                }
            }
            else if (_groupSpawnType == 2)
            {
                for (var index = 0; index < _groupSpawnCount && group.RemainingRead() >= sizeof(uint); index++)
                {
                    RemoveMonster(group.ReadUInt32());
                }
            }
        }
        finally
        {
            _groupSpawnBytes.SetLength(0);
            _groupSpawnCount = 0;
            _groupSpawnType = 0;
        }
    }

    private bool TryParseGroupSpawnEntity(Packet packet)
    {
        var start = packet.SeekRead(0, SeekOrigin.Current);
        if (packet.RemainingRead() < sizeof(uint))
            return false;

        var refObjId = packet.ReadUInt32();
        if (refObjId == uint.MaxValue)
        {
            packet.ReadUInt16(); // vSRO spell-area header
            packet.ReadUInt32(); // skill id
            packet.ReadUInt32(); // unique id
            SkipAbsolutePosition(packet);
            return true;
        }

        if (refObjId == uint.MaxValue - 1)
        {
            // Legacy field decoration entity used by the vSRO client parser.
            packet.ReadUInt32();
            packet.ReadUInt32();
            return true;
        }

        if (!RefManager.RefObjCommons.TryGetValue(unchecked((int)refObjId), out var common))
        {
            packet.SeekRead(start, SeekOrigin.Begin);
            return false;
        }

        if (common.TypeID1 == 1 && common.TypeID2 == 2 && common.TypeID3 == 1)
        {
            packet.SeekRead(start, SeekOrigin.Begin);
            return TryParseMonster(packet, isGroup: true);
        }

        switch (common.TypeID1)
        {
            case 1 when common.TypeID2 == 1:
                SkipPlayerSpawn(packet, common);
                return true;
            case 1 when common.TypeID2 == 2 && common.TypeID3 == 3:
                SkipCosSpawn(packet, common.TypeID4);
                return true;
            case 1 when common.TypeID2 == 2 && common.TypeID3 == 5:
                packet.ReadUInt32();
                packet.ReadUInt32();
                packet.ReadUInt16();
                SkipBionicDetails(packet);
                SkipNpcTalk(packet);
                packet.ReadUInt32();
                packet.ReadAscii();
                return true;
            case 1 when common.TypeID2 == 2:
                SkipBionicDetails(packet);
                SkipNpcTalk(packet);
                return true;
            case 3:
                SkipItemSpawn(packet, common, isGroup: true);
                return true;
            case 4:
                SkipPortalSpawn(packet);
                return true;
            default:
                packet.SeekRead(start, SeekOrigin.Begin);
                return false;
        }
    }

    private void SkipPlayerSpawn(Packet packet, KMTGuard.Database.Shard._RefObjCommon player)
    {
        packet.ReadUInt8(); // scale
        packet.ReadUInt8(); // hwan level (vSRO)
        packet.ReadUInt8(); // pvp cape (vSRO)
        packet.ReadUInt8(); // auto-invest
        packet.ReadUInt8(); // inventory size
        var itemCount = packet.ReadUInt8();
        var wearsJobSuit = false;
        for (var index = 0; index < itemCount; index++)
        {
            var itemRefId = packet.ReadUInt32();
            if (!RefManager.RefObjCommons.TryGetValue(unchecked((int)itemRefId), out var item))
                throw new InvalidDataException($"Unknown equipped item RefID {itemRefId} in player spawn.");
            if (item.TypeID2 == 1)
                packet.ReadUInt8(); // plus
            if (item.TypeID2 == 1 && item.TypeID3 == 7 && item.TypeID4 is not 4 and not 5)
                wearsJobSuit = true;
        }

        packet.ReadUInt8(); // avatar inventory size
        var avatarCount = packet.ReadUInt8();
        for (var index = 0; index < avatarCount; index++)
        {
            packet.ReadUInt32();
            packet.ReadUInt8();
        }

        if (packet.ReadUInt8() != 0)
        {
            var maskRefId = packet.ReadUInt32();
            if (RefManager.RefObjCommons.TryGetValue(unchecked((int)maskRefId), out var mask) &&
                (mask.TypeID1 == player.TypeID1 || mask.TypeID2 == player.TypeID2))
            {
                packet.ReadUInt8();
                var maskItems = packet.ReadUInt8();
                for (var index = 0; index < maskItems; index++)
                    packet.ReadUInt32();
            }
        }

        SkipBionicDetails(packet);
        packet.ReadAscii(); // character name
        packet.ReadUInt8(); // job type
        packet.ReadUInt8(); // job level (vSRO)
        packet.ReadUInt8(); // pvp state (vSRO)
        var onTransport = packet.ReadUInt8() != 0;
        packet.ReadUInt8(); // in combat
        if (onTransport)
            packet.ReadUInt32();
        packet.ReadUInt8(); // scroll mode
        var interactMode = packet.ReadUInt8();
        packet.ReadUInt8(); // legacy vSRO extension
        packet.ReadAscii(); // guild name

        if (!wearsJobSuit)
        {
            packet.ReadUInt32();
            packet.ReadAscii();
            packet.ReadUInt32();
            packet.ReadUInt32();
            packet.ReadUInt32();
            packet.ReadUInt8();
            packet.ReadUInt8();
        }

        if (interactMode == 4)
        {
            packet.ReadAscii();
            packet.ReadUInt32();
        }
        packet.ReadUInt8(); // equipment cooldown
        packet.ReadUInt8(); // PK flag
    }

    private void SkipCosSpawn(Packet packet, byte typeId4)
    {
        SkipBionicDetails(packet);
        SkipNpcTalk(packet);
        if (typeId4 is < 2 or > 9)
            return;

        if (typeId4 is 3 or 4 or 9)
            packet.ReadAscii();
        else if (typeId4 == 5)
            packet.ReadAscii();

        if (typeId4 is 2 or 3 or 4 or 5 or 6 or 9)
        {
            packet.ReadAscii();
            if (typeId4 is 2 or 3 or 4 or 5 or 9)
            {
                packet.ReadUInt8();
                if (typeId4 is 2 or 3 or 5 or 9)
                {
                    packet.ReadUInt8();
                    if (typeId4 == 5)
                        packet.ReadUInt32();
                }
            }
        }
        packet.ReadUInt32();
        if (typeId4 == 9)
            packet.ReadUInt8();
    }

    private static void SkipItemSpawn(
        Packet packet,
        KMTGuard.Database.Shard._RefObjCommon item,
        bool isGroup)
    {
        if (item.TypeID2 == 1)
            packet.ReadUInt8();
        else if (item.TypeID2 == 3 && item.TypeID3 == 5 && item.TypeID4 == 0)
            packet.ReadUInt32();
        else if (item.TypeID2 == 3 && item.TypeID3 is 8 or 9)
            packet.ReadAscii();

        packet.ReadUInt32();
        SkipAbsolutePosition(packet);
        if (packet.ReadUInt8() != 0)
            packet.ReadUInt32();
        packet.ReadUInt8();
        if (!isGroup)
        {
            packet.ReadUInt8();
            packet.ReadUInt32();
        }
    }

    private static void SkipPortalSpawn(Packet packet)
    {
        packet.ReadUInt32();
        SkipAbsolutePosition(packet);
        packet.ReadUInt8();
        var hasPortalDetails = packet.ReadUInt8();
        packet.ReadUInt8();
        var portalType = packet.ReadUInt8();
        if (portalType == 1)
        {
            packet.ReadUInt32();
            packet.ReadUInt32();
        }
        else if (portalType == 6)
        {
            packet.ReadAscii();
            packet.ReadUInt32();
        }
        if (hasPortalDetails == 1)
        {
            packet.ReadUInt32();
            packet.ReadUInt8();
        }
    }

    private void SkipBionicDetails(Packet packet)
    {
        packet.ReadUInt32();
        SkipAbsolutePosition(packet);
        var hasDestination = packet.ReadUInt8() != 0;
        packet.ReadUInt8(); // movement type
        if (hasDestination)
            SkipConditionalPosition(packet);
        else
        {
            packet.ReadUInt8();
            packet.ReadInt16();
        }
        packet.ReadUInt8(); // life
        packet.ReadUInt8(); // vSRO state extension
        packet.ReadUInt8(); // motion
        packet.ReadUInt8(); // body
        packet.ReadSingle();
        packet.ReadSingle();
        packet.ReadSingle();
        var buffCount = packet.ReadUInt8();
        for (var index = 0; index < buffCount; index++)
        {
            var skillId = packet.ReadUInt32();
            packet.ReadUInt32();
            if (_creatorFlagBuffSkillIds.Contains(skillId))
                packet.ReadUInt8();
        }
    }

    private static void SkipNpcTalk(Packet packet)
    {
        var flag = packet.ReadUInt8();
        if ((flag & 2) != 0)
        {
            var count = packet.ReadUInt8();
            packet.ReadUInt8Array(count);
        }
        if (flag == 6 && packet.ReadUInt8() == 1)
            packet.ReadUInt32();
    }

    private static void SkipAbsolutePosition(Packet packet)
    {
        packet.ReadUInt16();
        packet.ReadSingle();
        packet.ReadSingle();
        packet.ReadSingle();
        packet.ReadInt16();
    }

    private bool TryParseMonster(Packet packet, bool isGroup)
    {
        var start = packet.SeekRead(0, SeekOrigin.Current);
        var refObjId = packet.ReadUInt32();
        if (!RefManager.RefObjCommons.TryGetValue(unchecked((int)refObjId), out var common) ||
            common.TypeID1 != 1 || common.TypeID2 != 2 || common.TypeID3 != 1)
        {
            packet.SeekRead(start, SeekOrigin.Begin);
            return false;
        }

        var uniqueId = packet.ReadUInt32();
        var regionId = packet.ReadUInt16();
        var x = packet.ReadSingle();
        var z = packet.ReadSingle();
        var y = packet.ReadSingle();
        if (uniqueId == 0 || uniqueId == _selfUniqueId ||
            !float.IsFinite(x) || !float.IsFinite(y) || !float.IsFinite(z) ||
            !IsPlausibleSpawnPosition(regionId, x, y, z))
        {
            packet.SeekRead(start, SeekOrigin.Begin);
            return false;
        }
        packet.ReadInt16();
        var hasDestination = packet.ReadUInt8() != 0;
        packet.ReadUInt8();
        if (hasDestination)
            SkipConditionalPosition(packet);
        else
        {
            packet.ReadUInt8();
            packet.ReadInt16();
        }

        var lifeState = packet.ReadUInt8();
        if (lifeState > 4)
        {
            packet.SeekRead(start, SeekOrigin.Begin);
            return false;
        }
        packet.ReadUInt8(); // vSRO state extension
        packet.ReadUInt8(); // motion state
        packet.ReadUInt8(); // body state
        var walkSpeed = packet.ReadSingle();
        var runSpeed = packet.ReadSingle();
        var berserkSpeed = packet.ReadSingle();
        if (!IsPlausibleSpeed(walkSpeed) || !IsPlausibleSpeed(runSpeed) || !IsPlausibleSpeed(berserkSpeed))
        {
            packet.SeekRead(start, SeekOrigin.Begin);
            return false;
        }
        var buffCount = packet.ReadUInt8();
        if (buffCount > 64)
        {
            packet.SeekRead(start, SeekOrigin.Begin);
            return false;
        }
        for (var index = 0; index < buffCount; index++)
        {
            var buffSkillId = packet.ReadUInt32();
            packet.ReadUInt32();
            if (_creatorFlagBuffSkillIds.Contains(buffSkillId))
                packet.ReadUInt8();
        }
        var talkFlag = packet.ReadUInt8();
        if ((talkFlag & 2) != 0)
        {
            var optionCount = packet.ReadUInt8();
            if (optionCount > 32 || packet.RemainingRead() < optionCount)
            {
                packet.SeekRead(start, SeekOrigin.Begin);
                return false;
            }
            for (var option = 0; option < optionCount; option++)
                packet.ReadUInt8();
        }
        if (talkFlag == 6 && packet.ReadUInt8() == 1)
            packet.ReadUInt32();

        var spawnRarity = packet.ReadUInt8(); // normal/champion/giant/party variants
        if (spawnRarity > 8)
        {
            packet.SeekRead(start, SeekOrigin.Begin);
            return false;
        }
        if (common.TypeID4 is 2 or 3)
            packet.ReadUInt8();
        if (!isGroup)
            packet.ReadUInt8();

        _monsters[uniqueId] = new HuntMonster(
            uniqueId,
            common.CodeName128,
            common.Rarity == 3,
            lifeState != 2,
            regionId,
            x,
            y,
            z);
        return true;
    }

    private static bool IsPlausibleSpawnPosition(int regionId, float x, float y, float z)
    {
        if ((regionId & 0x8000) != 0)
            return Math.Abs(x) <= 10_000_000f && Math.Abs(y) <= 1_000_000f && Math.Abs(z) <= 10_000_000f;

        return regionId > 0 && x is >= -64f and <= 1984f && z is >= -64f and <= 1984f &&
               Math.Abs(y) <= 100_000f;
    }

    private static bool IsPlausibleSpeed(float speed) =>
        float.IsFinite(speed) && speed is >= 0f and <= 10_000f;

    private void HandleCosData(Packet packet)
    {
        if (packet.RemainingRead() < 8)
            return;

        var uniqueId = packet.ReadUInt32();
        var refObjId = packet.ReadUInt32();
        if (!RefManager.RefObjCommons.TryGetValue(unchecked((int)refObjId), out var common) ||
            common.TypeID1 != 1 || common.TypeID2 != 2 || common.TypeID3 != 3)
            return;

        uint health = 0;
        uint maxHealth = 0;
        if (packet.RemainingRead() >= 8)
        {
            health = packet.ReadUInt32();
            maxHealth = packet.ReadUInt32();
        }

        switch (common.TypeID4)
        {
            case 3: // vSRO growth/attack pet
                _attackPetUniqueId = uniqueId;
                _attackPetHp = health;
                _attackPetMaxHp = maxHealth;
                _attackPetDead = health == 0 && maxHealth > 0;
                _attackPetTargetUniqueId = 0;
                if (packet.RemainingRead() >= 11)
                {
                    packet.ReadUInt64(); // experience
                    packet.ReadUInt8();  // level
                    _attackPetHunger = packet.ReadUInt16();
                }
                _pendingPetSummon = PetSummonKind.None;
                _nextPetActionUtc = DateTime.UtcNow.AddMilliseconds(500);
                HuntLog.Information(
                    "Clientless {Character} confirmed attack pet {PetCode} ({PetUniqueId}).",
                    _account.CharacterName,
                    common.CodeName128,
                    uniqueId);
                break;

            case 4: // vSRO ability/grab pet
                _grabPetUniqueId = uniqueId;
                _pendingPetSummon = PetSummonKind.None;
                _nextPetActionUtc = DateTime.UtcNow.AddMilliseconds(500);
                HuntLog.Information(
                    "Clientless {Character} confirmed grab pet {PetCode} ({PetUniqueId}).",
                    _account.CharacterName,
                    common.CodeName128,
                    uniqueId);
                break;

            case 9: // fellow pets are also attack-capable on compatible servers
                _attackPetUniqueId = uniqueId;
                _attackPetHp = health;
                _attackPetMaxHp = maxHealth;
                _attackPetDead = health == 0 && maxHealth > 0;
                _attackPetTargetUniqueId = 0;
                _pendingPetSummon = PetSummonKind.None;
                _nextPetActionUtc = DateTime.UtcNow.AddMilliseconds(500);
                break;
        }
    }

    private void HandleCosUpdate(Packet packet)
    {
        if (packet.RemainingRead() < 5)
            return;

        var uniqueId = packet.ReadUInt32();
        var updateType = packet.ReadUInt8();
        if (uniqueId == _attackPetUniqueId)
        {
            if (updateType == 1)
            {
                _attackPetUniqueId = 0;
                _attackPetTargetUniqueId = 0;
                _nextPetActionUtc = DateTime.UtcNow.AddSeconds(2);
            }
            else if (updateType == 4 && packet.RemainingRead() >= 2)
            {
                _attackPetHunger = packet.ReadUInt16();
            }
        }
        else if (uniqueId == _grabPetUniqueId && updateType == 1)
        {
            _grabPetUniqueId = 0;
            _nextPetActionUtc = DateTime.UtcNow.AddSeconds(2);
        }
    }

    private void HandleLifeState(Packet packet)
    {
        var uniqueId = packet.ReadUInt32();
        var type = packet.ReadUInt8();
        var state = packet.ReadUInt8();
        if (type != 0)
            return;

        if (uniqueId == _selfUniqueId)
        {
            var wasDead = _dead;
            _dead = state == 2;
            if (_dead && !wasDead)
            {
                _deadSinceUtc = DateTime.UtcNow;
                _returnAfterDeath = false;
                _activeBuffSkillIds.Clear();
                HuntLog.Warning(
                    "Clientless {Character} received a confirmed dead life-state from vSRO GameServer; town recovery will start.",
                    _account.CharacterName);
            }
            else if (!_dead && wasDead)
            {
                _returnAfterDeath = false;
                HuntLog.Information(
                    "Clientless {Character} received a confirmed alive life-state after recovery.",
                    _account.CharacterName);
            }
            return;
        }

        if (uniqueId == _attackPetUniqueId && state == 2)
        {
            _attackPetDead = true;
            _attackPetUniqueId = 0;
            _attackPetTargetUniqueId = 0;
            _nextPetActionUtc = DateTime.UtcNow.AddSeconds(5);
            return;
        }

        if (_monsters.TryGetValue(uniqueId, out var monster) && state == 2)
        {
            _monsters[uniqueId] = monster with { Alive = false };
            if (_targetUniqueId == uniqueId)
                ReleaseDefeatedTarget(uniqueId);
        }
    }

    private void HandleHealthUpdate(Packet packet)
    {
        var uniqueId = packet.ReadUInt32();
        packet.ReadUInt16();
        var flags = packet.ReadUInt8();
        if ((flags & 1) != 0)
        {
            var hp = packet.ReadUInt32();
            if (uniqueId == _selfUniqueId)
            {
                _currentHp = hp;
                _maximumObservedHp = Math.Max(_maximumObservedHp, hp);
                if (hp == 0)
                {
                    if (!_dead)
                    {
                        _dead = true;
                        _deadSinceUtc = DateTime.UtcNow;
                        _returnAfterDeath = false;
                        _activeBuffSkillIds.Clear();
                        HuntLog.Warning(
                            "Clientless {Character} reached zero HP according to vSRO GameServer; town recovery will start.",
                            _account.CharacterName);
                    }
                }
                else if (_dead)
                {
                    _dead = false;
                    _returnAfterDeath = false;
                    HuntLog.Information(
                        "Clientless {Character} received confirmed positive HP after town recovery.",
                        _account.CharacterName);
                }
            }
            else if (uniqueId == _attackPetUniqueId)
            {
                _attackPetHp = hp;
                if (hp == 0)
                {
                    _attackPetDead = true;
                    _attackPetUniqueId = 0;
                    _attackPetTargetUniqueId = 0;
                    _nextPetActionUtc = DateTime.UtcNow.AddSeconds(5);
                }
            }
            else if (_monsters.TryGetValue(uniqueId, out var monster) && hp == 0)
            {
                _monsters[uniqueId] = monster with { Alive = false };
                if (_targetUniqueId == uniqueId)
                    ReleaseDefeatedTarget(uniqueId);
            }
        }
        if ((flags & 2) != 0)
        {
            var mp = packet.ReadUInt32();
            if (uniqueId == _selfUniqueId)
            {
                _currentMp = mp;
                _maximumObservedMp = Math.Max(_maximumObservedMp, mp);
            }
        }
    }

    private void HandleStatsUpdate(Packet packet)
    {
        // vSRO 0x303D: four attack values, four defence/rate values,
        // maximum HP/MP, then STR/INT. The old engine treated the login-time
        // current HP/MP as the maximum, so a high-level character created with
        // 200 HP never considered itself injured and did not use a potion.
        if (packet.RemainingRead() < 36)
            return;

        packet.ReadUInt32(); // physical attack min
        packet.ReadUInt32(); // physical attack max
        packet.ReadUInt32(); // magical attack min
        packet.ReadUInt32(); // magical attack max
        packet.ReadUInt16(); // physical defence
        packet.ReadUInt16(); // magical defence
        packet.ReadUInt16(); // hit rate
        packet.ReadUInt16(); // parry rate
        var maximumHp = packet.ReadUInt32();
        var maximumMp = packet.ReadUInt32();
        packet.ReadUInt16(); // STR
        packet.ReadUInt16(); // INT

        if (maximumHp > 0)
            _maximumObservedHp = maximumHp;
        if (maximumMp > 0)
            _maximumObservedMp = maximumMp;
    }

    private void HandleMovement(Packet packet)
    {
        var uniqueId = packet.ReadUInt32();
        var hasDestination = packet.ReadUInt8() != 0;
        (int Region, float X, float Y, float Z)? destination = null;
        if (hasDestination)
        {
            destination = ReadConditionalPosition(packet);
        }
        else
        {
            packet.ReadUInt8(); // spin/key-walk marker
            packet.ReadInt16(); // angle
        }

        (int Region, float X, float Y, float Z)? source = null;
        if (packet.RemainingRead() > 0 && packet.ReadUInt8() != 0)
            source = ReadMovementSource(packet);

        if (uniqueId == _selfUniqueId)
        {
            _moveAwaitingResponse = false;
            if (source.HasValue)
            {
                _currentRegion = source.Value.Region;
                _currentX = source.Value.X;
                _currentY = source.Value.Y;
                _currentZ = source.Value.Z;
            }

            if (!destination.HasValue)
            {
                _movementInProgress = false;
                return;
            }

            _moveSourceRegion = _currentRegion;
            _moveSourceX = _currentX;
            _moveSourceY = _currentY;
            _moveSourceZ = _currentZ;
            _moveDestinationRegion = destination.Value.Region;
            _moveDestinationX = destination.Value.X;
            _moveDestinationY = destination.Value.Y;
            _moveDestinationZ = destination.Value.Z;
            var now = DateTime.UtcNow;
            var distance = DistanceMeters(
                _moveSourceRegion,
                _moveSourceX,
                _moveSourceZ,
                _moveDestinationRegion,
                _moveDestinationX,
                _moveDestinationZ);
            _movementStartedUtc = now;
            _movementArrivalUtc = now.Add(EstimateMovementDuration(distance, _runSpeedMetersPerSecond));
            _movementStuckDeadlineUtc = _movementArrivalUtc.AddSeconds(
                Math.Clamp(_policy.StuckSeconds, (short)5, (short)120));
            _movementInProgress = true;
        }
        else if (_monsters.TryGetValue(uniqueId, out var monster))
        {
            // The B021 destination describes where the entity is going, not
            // where it currently stands. Prefer the authoritative source when
            // present so range checks do not attack a position the monster has
            // not reached yet.
            var position = source ?? destination;
            if (position.HasValue)
            {
                _monsters[uniqueId] = monster with
                {
                    RegionID = position.Value.Region,
                    X = position.Value.X,
                    Y = position.Value.Y,
                    Z = position.Value.Z
                };
            }
        }
    }

    private void HandleSelectionResponse(Packet packet)
    {
        var success = packet.ReadUInt8() == 1;
        if (!success)
        {
            HuntLog.Debug(
                "Clientless {Character} target selection was rejected for target {TargetId}.",
                _account.CharacterName,
                _targetUniqueId);
            ReleaseTarget();
            return;
        }
        var uniqueId = packet.ReadUInt32();
        _targetSelected = uniqueId == _targetUniqueId;
        if (_targetSelected)
        {
            _combatFeedback = "Target selected; preparing learned attack skills";
            HuntLog.Debug(
                "Clientless {Character} selected target {TargetId}.",
                _account.CharacterName,
                uniqueId);
        }
    }

    private void HandleAuthoritativePosition(Packet packet, bool uniqueIdFirst, bool stopsMovement)
    {
        uint uniqueId;
        (int Region, float X, float Y, float Z) position;
        if (uniqueIdFirst)
        {
            uniqueId = packet.ReadUInt32();
            position = ReadAbsolutePosition(packet);
        }
        else
        {
            position = ReadAbsolutePosition(packet);
            uniqueId = packet.ReadUInt32();
        }

        if (uniqueId == _selfUniqueId)
        {
            _currentRegion = position.Region;
            _currentX = position.X;
            _currentY = position.Y;
            _currentZ = position.Z;
            if (stopsMovement)
            {
                _moveAwaitingResponse = false;
                _movementInProgress = false;
            }
            else if (_movementInProgress)
            {
                _moveSourceRegion = position.Region;
                _moveSourceX = position.X;
                _moveSourceY = position.Y;
                _moveSourceZ = position.Z;
                var now = DateTime.UtcNow;
                var remaining = DistanceMeters(
                    position.Region,
                    position.X,
                    position.Z,
                    _moveDestinationRegion,
                    _moveDestinationX,
                    _moveDestinationZ);
                _movementStartedUtc = now;
                _movementArrivalUtc = now.Add(EstimateMovementDuration(remaining, _runSpeedMetersPerSecond));
                _movementStuckDeadlineUtc = _movementArrivalUtc.AddSeconds(
                    Math.Clamp(_policy.StuckSeconds, (short)5, (short)120));
            }
        }
        else if (_monsters.TryGetValue(uniqueId, out var monster))
        {
            _monsters[uniqueId] = monster with
            {
                RegionID = position.Region,
                X = position.X,
                Y = position.Y,
                Z = position.Z
            };
        }
    }

    private void HandleMoveSpeed(Packet packet)
    {
        var uniqueId = packet.ReadUInt32();
        packet.ReadSingle(); // walking speed
        var runSpeed = packet.ReadSingle();
        if (uniqueId == _selfUniqueId && float.IsFinite(runSpeed) && runSpeed > 0)
            _runSpeedMetersPerSecond = Math.Clamp(runSpeed / 10d, 1d, 50d);
    }

    private void HandleSkillCastResponse(Packet packet)
    {
        if (!ClientlessHuntProtocol.TryReadSkillCastFeedback(
                packet,
                _selfUniqueId,
                out var feedback))
            return;

        if (!feedback.Accepted)
        {
            if (_pendingAction != PendingCombatAction.None)
            {
                RegisterPendingActionFailure(
                    feedback.ErrorCode,
                    DescribeSkillError(feedback.ErrorCode),
                    DateTime.UtcNow);
            }
            return;
        }

        if (!feedback.OwnAction || _pendingAction == PendingCombatAction.None)
            return;

        if ((_pendingAction is PendingCombatAction.Skill or PendingCombatAction.Buff) &&
            _pendingSkill != null &&
            feedback.SkillId != 0 &&
            feedback.SkillId != _pendingSkill.ID)
            return;

        CompletePendingAction(DateTime.UtcNow);
    }

    private void HandleBuffApplied(Packet packet)
    {
        if (!ClientlessHuntProtocol.TryReadBuffApplied(packet, out var feedback) ||
            feedback.TargetId != _selfUniqueId || feedback.SkillId == 0)
            return;

        _activeBuffSkillIds.Add(feedback.SkillId);
        _buffConfirmationDeadlines.Remove(feedback.SkillId);
        _buffFailureCounts.Remove(feedback.SkillId);
        if (feedback.Token != 0)
            _buffSkillByToken[feedback.Token] = feedback.SkillId;

        if (!_hasConfirmedSelfBuff)
        {
            _hasConfirmedSelfBuff = true;
            HuntLog.Information(
                "Clientless {Character} confirmed its first active self buff {SkillId} through vSRO B0BD.",
                _account.CharacterName,
                feedback.SkillId);
        }
    }

    private void HandleBuffRemoved(Packet packet)
    {
        if (!ClientlessHuntProtocol.TryReadRemovedBuffTokens(packet, out var tokens))
            return;

        foreach (var token in tokens)
        {
            if (!_buffSkillByToken.Remove(token, out var skillId))
                continue;

            _activeBuffSkillIds.Remove(skillId);
            _skillReadyAtUtc.Remove(skillId);
        }
    }

    private void BeginPendingAction(
        PendingCombatAction action,
        DateTime now,
        ClientlessCombatSkill? skill = null)
    {
        _pendingAction = action;
        _pendingSkill = skill;
        _pendingActionSentUtc = now;
    }

    private void ExpireUnconfirmedBuffs(DateTime now)
    {
        foreach (var skillId in _buffConfirmationDeadlines
                     .Where(item => now >= item.Value)
                     .Select(item => item.Key)
                     .ToArray())
        {
            _buffConfirmationDeadlines.Remove(skillId);
            var failureCount = _buffFailureCounts.GetValueOrDefault(skillId) + 1;
            _buffFailureCounts[skillId] = failureCount;
            _skillReadyAtUtc[skillId] = now.AddSeconds(5);
            _nextBuffAttemptUtc = now.AddSeconds(2);
            if (failureCount >= 4)
                _disabledBuffSkillIds.Add(skillId);

            HuntLog.Warning(
                "Clientless {Character} did not receive vSRO B0BD activation for accepted self buff {SkillId} (attempt {Attempt}); it will {Outcome}.",
                _account.CharacterName,
                skillId,
                failureCount,
                failureCount >= 4 ? "be skipped for this session" : "be retried");
        }
    }

    private void CompletePendingAction(DateTime now)
    {
        var completedAction = _pendingAction;
        var completedSkill = _pendingSkill;
        if ((completedAction is PendingCombatAction.Skill or PendingCombatAction.Buff) && completedSkill != null)
        {
            var cooldown = Math.Clamp(completedSkill.CooldownMs, 700, 120000);
            _skillReadyAtUtc[completedSkill.ID] = now.AddMilliseconds(cooldown + 150);
            if (completedAction == PendingCombatAction.Buff)
            {
                _buffFailureCounts.Remove(completedSkill.ID);
                _buffConfirmationDeadlines[completedSkill.ID] = now.AddSeconds(6);
                _combatFeedback = $"Self buff accepted; waiting for active-buff confirmation: {completedSkill.CodeName128}";
                HuntLog.Debug(
                    "Clientless {Character} self buff {Skill} ({SkillId}) was accepted through vSRO B070 and is awaiting B0BD activation.",
                    _account.CharacterName,
                    completedSkill.CodeName128,
                    completedSkill.ID);
            }
            else
            {
                _attackSkillFailureCounts.Remove(completedSkill.ID);
                _combatFeedback = $"Attack skill accepted: {completedSkill.CodeName128}";
                if (!_hasConfirmedAttackSkill)
                {
                    _hasConfirmedAttackSkill = true;
                    HuntLog.Information(
                        "Clientless {Character} confirmed its first attack skill {Skill} ({SkillId}) through vSRO B070.",
                        _account.CharacterName,
                        completedSkill.CodeName128,
                        completedSkill.ID);
                }
                else
                {
                    HuntLog.Debug(
                        "Clientless {Character} attack skill {Skill} ({SkillId}) was accepted through vSRO B070.",
                        _account.CharacterName,
                        completedSkill.CodeName128,
                        completedSkill.ID);
                }
            }
        }
        else if (completedAction == PendingCombatAction.BasicAttack)
        {
            _combatFeedback = "Basic attack accepted by GameServer";
        }

        _pendingAction = PendingCombatAction.None;
        _pendingSkill = null;
        var actionDelay = completedSkill == null
            ? 700
            : Math.Clamp(completedSkill.CastDurationMs + 250, 700, 15000);
        _nextActionUtc = now.AddMilliseconds(actionDelay);
    }

    private void RegisterPendingActionFailure(ushort errorCode, string reason, DateTime now)
    {
        var failedAction = _pendingAction;
        var failedSkill = _pendingSkill;
        _pendingAction = PendingCombatAction.None;
        _pendingSkill = null;
        _nextActionUtc = now.AddMilliseconds(250);

        if ((failedAction is PendingCombatAction.Skill or PendingCombatAction.Buff) && failedSkill != null)
        {
            int failureCount;
            if (failedAction == PendingCombatAction.Skill)
            {
                var transientFailure = errorCode is 0 or 0x05 or 0x06 or 0x0C or 0x10;
                failureCount = transientFailure
                    ? _attackSkillFailureCounts.GetValueOrDefault(failedSkill.ID)
                    : _attackSkillFailureCounts.GetValueOrDefault(failedSkill.ID) + 1;
                _attackSkillFailureCounts[failedSkill.ID] = failureCount;
            }
            else
            {
                var transientFailure = errorCode is 0 or 0x05 or 0x0C;
                failureCount = transientFailure
                    ? _buffFailureCounts.GetValueOrDefault(failedSkill.ID)
                    : _buffFailureCounts.GetValueOrDefault(failedSkill.ID) + 1;
            }
            var retryDelay = errorCode switch
            {
                0x05 => TimeSpan.FromSeconds(2),
                0x06 => TimeSpan.FromMilliseconds(500),
                0x0C => TimeSpan.FromMilliseconds(1200),
                0x0E => TimeSpan.FromSeconds(30),
                0x10 => TimeSpan.FromMilliseconds(750),
                _ => TimeSpan.FromSeconds(3)
            };
            _skillReadyAtUtc[failedSkill.ID] = now.Add(retryDelay);
            if (failedAction == PendingCombatAction.Skill)
            {
                if ((errorCode == 0x0E || (errorCode == 0x13 && failureCount >= 2) || failureCount >= 5) &&
                    _disabledAttackSkillIds.Add(failedSkill.ID))
                {
                    HuntLog.Warning(
                        "Clientless {Character} disabled incompatible attack skill {Skill} ({SkillId}) for this session after {FailureCount} hard failure(s); the next learned damage skill will be used.",
                        _account.CharacterName,
                        failedSkill.CodeName128,
                        failedSkill.ID,
                        failureCount);
                }
            }
            else
            {
                _nextBuffAttemptUtc = now.AddSeconds(5);
                _buffFailureCounts[failedSkill.ID] = failureCount;
                if (failureCount >= 4 && _disabledBuffSkillIds.Add(failedSkill.ID))
                {
                    HuntLog.Warning(
                        "Clientless {Character} disabled incompatible self buff {Skill} ({SkillId}) for this session after {FailureCount} failed confirmations; combat will continue.",
                        _account.CharacterName,
                        failedSkill.CodeName128,
                        failedSkill.ID,
                        failureCount);
                }
            }

            if (failedAction == PendingCombatAction.Skill && errorCode is 0x06 or 0x10)
            {
                // Invalid/obstructed targets are target-position problems, not
                // proof that a learned skill is incompatible with the weapon.
                ReleaseTarget();
                _nextPatrolUtc = now;
            }
            _combatFeedback = failedAction == PendingCombatAction.Buff
                ? $"Self buff rejected ({reason}); combat remains active"
                : $"Attack skill rejected ({reason}); trying the next learned damage skill";

            if (failureCount == 1 || failureCount % 10 == 0)
            {
                HuntLog.Warning(
                    "Clientless {Character} {ActionKind} {Skill} ({SkillId}) was rejected by vSRO GameServer: {Reason} (0x{ErrorCode:X4}).",
                    _account.CharacterName,
                    failedAction == PendingCombatAction.Buff ? "self buff" : "attack skill",
                    failedSkill.CodeName128,
                    failedSkill.ID,
                    reason,
                    errorCode);
            }
            else
            {
                HuntLog.Debug(
                    "Clientless {Character} {ActionKind} {Skill} ({SkillId}) rejected: {Reason} (0x{ErrorCode:X4}).",
                    _account.CharacterName,
                    failedAction == PendingCombatAction.Buff ? "self buff" : "attack skill",
                    failedSkill.CodeName128,
                    failedSkill.ID,
                    reason,
                    errorCode);
            }
            return;
        }

        if (failedAction == PendingCombatAction.BasicAttack)
        {
            _combatFeedback = $"Basic attack was not confirmed ({reason}); retrying";
            HuntLog.Debug(
                "Clientless {Character} basic attack was not confirmed: {Reason} (0x{ErrorCode:X4}).",
                _account.CharacterName,
                reason,
                errorCode);
        }
    }

    private TimeSpan PendingActionFeedbackTimeout()
    {
        if (_pendingSkill == null)
            return TimeSpan.FromMilliseconds(2500);

        // A vSRO B070 accepts or rejects a self-buff at action start. Keeping
        // an idle buff pending for its full animation time can delay combat
        // after a monster spawns, so support actions use a short hard limit.
        if (_pendingAction == PendingCombatAction.Buff)
            return TimeSpan.FromMilliseconds(3500);

        return TimeSpan.FromMilliseconds(
            Math.Clamp(_pendingSkill.CastDurationMs + 2500, 2500, 30000));
    }

    private string PendingActionDescription(string targetName) => _pendingAction switch
    {
        PendingCombatAction.Buff => _pendingSkill == null ? "Casting self buff" : $"Casting self buff: {_pendingSkill.CodeName128}",
        PendingCombatAction.Skill => $"Using learned skill against {targetName}",
        PendingCombatAction.BasicAttack => $"Basic attack fallback: {targetName}",
        _ => FightDescription(targetName)
    };

    private string FightDescription(string targetName) =>
        string.IsNullOrWhiteSpace(_combatFeedback) ? targetName : _combatFeedback;

    private static string DescribeSkillError(ushort errorCode) => errorCode switch
    {
        0x04 => "action is not permitted in the current state",
        0x05 => "skill cooldown is still active",
        0x06 => "invalid or unavailable target",
        0x0C => "another skill action is still running",
        0x0E => "missing ammunition",
        0x10 => "target is behind an obstacle",
        0x13 => "skill or equipment requirements are not satisfied",
        0 => "no response",
        _ => $"GameServer error 0x{errorCode:X2}"
    };

    private void RemoveMonster(uint uniqueId)
    {
        _monsters.Remove(uniqueId);
        if (_targetUniqueId == uniqueId)
            ReleaseDefeatedTarget(uniqueId);
        else
            NormalTargetOwners.TryRemove(uniqueId, out _);
    }

    private void ReleaseDefeatedTarget(uint uniqueId)
    {
        ReleaseTarget();
        _nextTargetAcquireUtc = DateTime.UtcNow.AddMilliseconds(
            CalculateTargetReactionMilliseconds(_account.ID, uniqueId));
    }

    internal static int CalculateTargetReactionMilliseconds(int accountId, uint defeatedTargetUniqueId)
    {
        // A short stable per-character variation removes the mechanical
        // synchronized look without adding a visible post-kill pause.
        var seed = unchecked(((uint)accountId * 1103515245u) ^ defeatedTargetUniqueId);
        return 80 + (int)(seed % 121u);
    }

    private void ReleaseTarget()
    {
        if (_targetUniqueId != 0)
        {
            if (NormalTargetOwners.TryGetValue(_targetUniqueId, out var owner) && owner == _account.ID)
                NormalTargetOwners.TryRemove(_targetUniqueId, out _);
        }
        _targetUniqueId = 0;
        _targetSelected = false;
        _attackPetTargetUniqueId = 0;
        _selectionAttempts = 0;
        _nextSelectUtc = DateTime.MinValue;
        _pendingAction = PendingCombatAction.None;
        _pendingSkill = null;
        _combatFeedback = string.Empty;
    }

    private static Task SetStatusAsync(string status, string message, string? targetName = null)
    {
        // Live hunt telemetry is intentionally not persisted. Connection state and
        // genuine failures remain available without generating per-action SQL work.
        return Task.CompletedTask;
    }

    private static Packet Clone(Packet source)
    {
        var copy = new Packet(source.Opcode, false, false, source.GetBytes());
        copy.ToReadOnly();
        return copy;
    }

    private static void SkipConditionalPosition(Packet packet)
    {
        var region = packet.ReadUInt16();
        if ((region & 0x8000) == 0)
        {
            packet.ReadInt16();
            packet.ReadInt16();
            packet.ReadInt16();
        }
        else
        {
            packet.ReadInt32();
            packet.ReadInt32();
            packet.ReadInt32();
        }
    }

    private static (int Region, float X, float Y, float Z) ReadConditionalPosition(Packet packet)
    {
        var region = packet.ReadUInt16();
        if ((region & 0x8000) == 0)
        {
            var x = packet.ReadInt16();
            var z = packet.ReadInt16();
            var y = packet.ReadInt16();
            return (region, x, y, z);
        }
        var dungeonX = packet.ReadInt32();
        var dungeonZ = packet.ReadInt32();
        var dungeonY = packet.ReadInt32();
        return (region, dungeonX, dungeonY, dungeonZ);
    }

    private static (int Region, float X, float Y, float Z) ReadAbsolutePosition(Packet packet)
    {
        var region = packet.ReadUInt16();
        var x = packet.ReadSingle();
        var z = packet.ReadSingle();
        var y = packet.ReadSingle();
        packet.ReadInt16(); // angle
        return (region, x, y, z);
    }

    internal static (int Region, float X, float Y, float Z) ReadMovementSource(Packet packet)
    {
        var region = packet.ReadUInt16();
        if ((region & 0x8000) == 0)
        {
            // B021 source coordinates use tenths for the two horizontal axes,
            // while the vertical coordinate is a float. Normalize them to the
            // raw sector-offset units used by spawn and destination packets.
            var x = packet.ReadInt16() / 10f;
            var z = packet.ReadSingle();
            var y = packet.ReadInt16() / 10f;
            return (region, x, y, z);
        }

        var dungeonX = packet.ReadInt32() / 10f;
        var dungeonZ = packet.ReadSingle();
        var dungeonY = packet.ReadInt32() / 10f;
        return (region, dungeonX, dungeonY, dungeonZ);
    }

    private static TimeSpan EstimateMovementDuration(double distanceMeters, double speedMetersPerSecond)
    {
        if (!double.IsFinite(distanceMeters) || distanceMeters <= 0.25)
            return TimeSpan.FromMilliseconds(350);

        return TimeSpan.FromSeconds(Math.Clamp(
            distanceMeters / Math.Clamp(speedMetersPerSecond, 1d, 50d),
            0.35,
            120));
    }

    private void UpdateEstimatedMovement(DateTime now)
    {
        var totalMilliseconds = Math.Max(1, (_movementArrivalUtc - _movementStartedUtc).TotalMilliseconds);
        var elapsedMilliseconds = Math.Clamp((now - _movementStartedUtc).TotalMilliseconds, 0, totalMilliseconds);
        var progress = (float)(elapsedMilliseconds / totalMilliseconds);

        if ((_moveSourceRegion & 0x8000) != 0 || (_moveDestinationRegion & 0x8000) != 0)
        {
            if (_moveSourceRegion != _moveDestinationRegion)
                return;

            _currentRegion = _moveSourceRegion;
            _currentX = Lerp(_moveSourceX, _moveDestinationX, progress);
            _currentY = Lerp(_moveSourceY, _moveDestinationY, progress);
            _currentZ = Lerp(_moveSourceZ, _moveDestinationZ, progress);
            return;
        }

        var sourceWorld = ToWorldRaw(_moveSourceRegion, _moveSourceX, _moveSourceZ);
        var destinationWorld = ToWorldRaw(_moveDestinationRegion, _moveDestinationX, _moveDestinationZ);
        var position = FromWorldRaw(
            Lerp(sourceWorld.X, destinationWorld.X, progress),
            Lerp(sourceWorld.Z, destinationWorld.Z, progress),
            Lerp(_moveSourceY, _moveDestinationY, progress));
        _currentRegion = position.Region;
        _currentX = position.X;
        _currentY = position.Y;
        _currentZ = position.Z;
    }

    private void CompleteEstimatedMovement()
    {
        _movementInProgress = false;
        _currentRegion = _moveDestinationRegion;
        _currentX = _moveDestinationX;
        _currentY = _moveDestinationY;
        _currentZ = _moveDestinationZ;
    }

    private static (int Region, float X, float Y, float Z) NormalizeFieldPosition(
        int region,
        float x,
        float y,
        float z)
    {
        if ((region & 0x8000) != 0)
            return (region, x, y, z);

        var world = ToWorldRaw(region, x, z);
        return FromWorldRaw(world.X, world.Z, y);
    }

    private static (float X, float Z) ToWorldRaw(int region, float x, float z) =>
        (((region & 0xFF) * 1920f) + x, (((region >> 8) & 0xFF) * 1920f) + z);

    private static (int Region, float X, float Y, float Z) FromWorldRaw(float worldX, float worldZ, float y)
    {
        const float sectorSize = 1920f;
        var sectorX = Math.Clamp((int)MathF.Floor(worldX / sectorSize), 0, byte.MaxValue);
        var sectorZ = Math.Clamp((int)MathF.Floor(worldZ / sectorSize), 0, byte.MaxValue);
        var localX = Math.Clamp(worldX - (sectorX * sectorSize), 0f, sectorSize - 0.01f);
        var localZ = Math.Clamp(worldZ - (sectorZ * sectorSize), 0f, sectorSize - 0.01f);
        return ((sectorZ << 8) | sectorX, localX, y, localZ);
    }

    private static float Lerp(float from, float to, float progress) => from + ((to - from) * progress);

    private static double DistanceMeters(int leftRegion, float leftX, float leftZ, int rightRegion, float rightX, float rightZ)
    {
        if ((leftRegion & 0x8000) != 0 || (rightRegion & 0x8000) != 0)
        {
            if (leftRegion != rightRegion)
                return double.MaxValue;
            var dungeonDx = leftX - rightX;
            var dungeonDz = leftZ - rightZ;
            return Math.Sqrt((dungeonDx * dungeonDx) + (dungeonDz * dungeonDz)) / 10d;
        }

        var leftSectorX = leftRegion & 0xFF;
        var leftSectorZ = (leftRegion >> 8) & 0xFF;
        var rightSectorX = rightRegion & 0xFF;
        var rightSectorZ = (rightRegion >> 8) & 0xFF;
        var dx = ((leftSectorX - rightSectorX) * 1920d) + leftX - rightX;
        var dz = ((leftSectorZ - rightSectorZ) * 1920d) + leftZ - rightZ;
        return Math.Sqrt((dx * dx) + (dz * dz)) / 10d;
    }

    private sealed record HuntMonster(
        uint UniqueId,
        string Name,
        bool IsUnique,
        bool Alive,
        int RegionID,
        float X,
        float Y,
        float Z);

    private enum PendingCombatAction
    {
        None,
        Skill,
        Buff,
        BasicAttack
    }

    private enum MpPreparationResult
    {
        Ready,
        PotionSent,
        Blocked
    }

    private enum TeleportState
    {
        Idle,
        RequestSent,
        Loading,
        Ready
    }

    private enum TeleportPurpose
    {
        None,
        RespawnTown,
        Town,
        HuntArea
    }

    private enum PetSummonKind
    {
        None,
        Attack,
        Grab
    }

    internal uint SelfUniqueId => _selfUniqueId;
}
