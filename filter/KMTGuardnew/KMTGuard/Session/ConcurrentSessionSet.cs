using System.Collections;
using System.Collections.Concurrent;

namespace KMTGuard.SessionManager;

/// <summary>
/// Thread-safe session registry with self-validating indexes for the stable
/// player identifiers used by hot packet and event paths.
/// </summary>
public sealed class ConcurrentSessionSet : IEnumerable<ISession>
{
    private readonly ConcurrentDictionary<Guid, ISession> _sessions = new();
    private readonly ConcurrentDictionary<int, ISession> _byCharId = new();
    private readonly ConcurrentDictionary<uint, ISession> _byUniqueCharId = new();
    private readonly ConcurrentDictionary<string, ISession> _byCharName =
        new(StringComparer.OrdinalIgnoreCase);
    private readonly ConcurrentDictionary<Guid, SessionIndexKeys> _indexKeys = new();
    private readonly object _indexSync = new();

    private readonly record struct SessionIndexKeys(int CharId, uint UniqueCharId, string CharName);

    public int Count => _sessions.Count;

    public bool Add(ISession session)
    {
        ArgumentNullException.ThrowIfNull(session);
        if (!_sessions.TryAdd(session.ClientGuid, session))
            return false;

        RefreshIndexes(session);
        return true;
    }

    public bool Remove(ISession session)
    {
        if (session == null)
            return false;

        if (!_sessions.TryRemove(session.ClientGuid, out var removedSession))
            return false;

        RemoveIndexes(removedSession);
        return true;
    }

    public int RemoveWhere(Predicate<ISession> predicate)
    {
        ArgumentNullException.ThrowIfNull(predicate);

        var removed = 0;
        foreach (var pair in _sessions.ToArray())
        {
            if (!predicate(pair.Value) || !_sessions.TryRemove(pair.Key, out var removedSession))
                continue;

            RemoveIndexes(removedSession);
            removed++;
        }

        return removed;
    }

    public ISession[] ToArray() => _sessions.Select(static pair => pair.Value).ToArray();

    public int CountWhere(Func<ISession, bool> predicate)
    {
        ArgumentNullException.ThrowIfNull(predicate);

        var count = 0;
        foreach (var pair in _sessions)
        {
            var session = pair.Value;
            if (predicate(session))
                count++;
        }

        return count;
    }

    public ISession? FindByCharId(int charId, Func<ISession, bool>? predicate = null)
    {
        if (charId <= 0)
            return null;

        if (_byCharId.TryGetValue(charId, out var indexed))
        {
            if (IsCurrent(indexed) && indexed.SessionData.Charid == charId)
            {
                if (predicate == null || predicate(indexed))
                    return indexed;
            }
            else
            {
                TryRemoveIndex(_byCharId, charId, indexed);
            }
        }

        foreach (var pair in _sessions)
        {
            var session = pair.Value;
            if (session.SessionData.Charid != charId || (predicate != null && !predicate(session)))
                continue;

            RefreshIndexes(session);
            return session;
        }

        return null;
    }

    public ISession? FindByUniqueCharId(uint uniqueCharId, Func<ISession, bool>? predicate = null)
    {
        if (uniqueCharId == 0)
            return null;

        if (_byUniqueCharId.TryGetValue(uniqueCharId, out var indexed))
        {
            if (IsCurrent(indexed) && indexed.SessionData.UniqueCharId == uniqueCharId)
            {
                if (predicate == null || predicate(indexed))
                    return indexed;
            }
            else
            {
                TryRemoveIndex(_byUniqueCharId, uniqueCharId, indexed);
            }
        }

        foreach (var pair in _sessions)
        {
            var session = pair.Value;
            if (session.SessionData.UniqueCharId != uniqueCharId || (predicate != null && !predicate(session)))
                continue;

            RefreshIndexes(session);
            return session;
        }

        return null;
    }

    public ISession? FindByCharName(string? charName, Func<ISession, bool>? predicate = null)
    {
        if (string.IsNullOrWhiteSpace(charName))
            return null;

        if (_byCharName.TryGetValue(charName, out var indexed))
        {
            if (IsCurrent(indexed) && string.Equals(indexed.SessionData.Charname, charName, StringComparison.OrdinalIgnoreCase))
            {
                if (predicate == null || predicate(indexed))
                    return indexed;
            }
            else
            {
                TryRemoveIndex(_byCharName, charName, indexed);
            }
        }

        foreach (var pair in _sessions)
        {
            var session = pair.Value;
            if (!string.Equals(session.SessionData.Charname, charName, StringComparison.OrdinalIgnoreCase) ||
                (predicate != null && !predicate(session)))
            {
                continue;
            }

            RefreshIndexes(session);
            return session;
        }

        return null;
    }

    public void RefreshIndexes(ISession session)
    {
        ArgumentNullException.ThrowIfNull(session);

        lock (_indexSync)
        {
            if (!IsCurrent(session))
                return;

            var current = new SessionIndexKeys(
                session.SessionData.Charid,
                session.SessionData.UniqueCharId,
                session.SessionData.Charname ?? string.Empty);

            if (_indexKeys.TryGetValue(session.ClientGuid, out var previous))
                RemoveIndexKeys(session, previous);

            if (current.CharId > 0)
                _byCharId[current.CharId] = session;
            if (current.UniqueCharId > 0)
                _byUniqueCharId[current.UniqueCharId] = session;
            if (!string.IsNullOrWhiteSpace(current.CharName))
                _byCharName[current.CharName] = session;

            _indexKeys[session.ClientGuid] = current;
        }
    }

    public IEnumerator<ISession> GetEnumerator()
    {
        foreach (var pair in _sessions)
            yield return pair.Value;
    }

    IEnumerator IEnumerable.GetEnumerator() => GetEnumerator();

    private bool IsCurrent(ISession session)
    {
        return _sessions.TryGetValue(session.ClientGuid, out var current) &&
               ReferenceEquals(current, session);
    }

    private void RemoveIndexes(ISession session)
    {
        lock (_indexSync)
        {
            if (_indexKeys.TryRemove(session.ClientGuid, out var keys))
                RemoveIndexKeys(session, keys);
        }
    }

    private void RemoveIndexKeys(ISession session, SessionIndexKeys keys)
    {
        if (keys.CharId > 0)
            TryRemoveIndex(_byCharId, keys.CharId, session);
        if (keys.UniqueCharId > 0)
            TryRemoveIndex(_byUniqueCharId, keys.UniqueCharId, session);
        if (!string.IsNullOrWhiteSpace(keys.CharName))
            TryRemoveIndex(_byCharName, keys.CharName, session);
    }

    private static void TryRemoveIndex<TKey>(
        ConcurrentDictionary<TKey, ISession> index,
        TKey key,
        ISession expectedSession)
        where TKey : notnull
    {
        if (index.TryGetValue(key, out var current) && ReferenceEquals(current, expectedSession))
            index.TryRemove(key, out _);
    }
}
