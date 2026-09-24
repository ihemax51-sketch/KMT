#pragma once

namespace LiveDpsMath
{
    inline unsigned long long Accumulate(unsigned long long total,
                                         unsigned int previous,
                                         unsigned int current)
    {
        if (total == 0) return current;
        if (current >= previous) return total + (current - previous);
        return total + (0xFFFFFFFFULL - previous) + 1ULL + current;
    }

    inline unsigned int ToLegacyWire(unsigned long long value)
    {
        return value > 0xFFFFFFFFULL ? 0xFFFFFFFFU : static_cast<unsigned int>(value);
    }
}
