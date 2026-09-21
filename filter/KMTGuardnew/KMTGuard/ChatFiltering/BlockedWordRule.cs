namespace KMTGuard.ChatFiltering
{
    public readonly struct BlockedWordRule
    {
        public readonly string Word;
        public readonly byte MatchMode;

        public BlockedWordRule(string word, byte matchMode)
        {
            Word = word;
            MatchMode = matchMode;
        }
    }
}
