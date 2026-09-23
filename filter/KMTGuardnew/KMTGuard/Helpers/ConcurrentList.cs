using System.Collections;

namespace KMTGuard.Helpers;

public sealed class ConcurrentList<T> : IEnumerable<T>
{
    private readonly object _sync = new();
    private readonly List<T> _items = new();

    public int Count { get { lock (_sync) return _items.Count; } }

    public void Add(T item) { lock (_sync) _items.Add(item); }
    public bool Remove(T item) { lock (_sync) return _items.Remove(item); }
    public bool Contains(T item) { lock (_sync) return _items.Contains(item); }
    public void Clear() { lock (_sync) _items.Clear(); }
    public T[] Snapshot() { lock (_sync) return _items.ToArray(); }
    public IEnumerator<T> GetEnumerator() => ((IEnumerable<T>)Snapshot()).GetEnumerator();
    IEnumerator IEnumerable.GetEnumerator() => GetEnumerator();
}
