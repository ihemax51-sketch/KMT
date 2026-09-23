#pragma once

#include "MsgStreamBuffer.h"
#include <limits.h>

class SafePacketReader {
public:
    explicit SafePacketReader(CMsgStreamBuffer& message) : m_message(message), m_valid(true) {}

    size_t Remaining() const { return m_valid ? m_message.Remaining() : 0; }

    template <typename T> bool CanRead() const { return Remaining() >= sizeof(T); }

    template <typename T> bool Read(T& value) {
        if (!m_valid || !m_message.TryReadBytes(&value, sizeof(T)))
            return Fail();
        return true;
    }

    bool ReadString(std::n_string& value, size_t maximumCharacters) {
        WORD length = 0;
        if (!Read(length) || length > maximumCharacters || Remaining() < length)
            return Fail();
        value.resize(length);
        return length == 0 || m_message.TryReadBytes(&value[0], length);
    }

    bool ReadWString(std::n_wstring& value, size_t maximumCharacters) {
        WORD length = 0;
        if (!Read(length) || length > maximumCharacters ||
            length > static_cast<size_t>(UINT_MAX / sizeof(wchar_t)) ||
            Remaining() < length * sizeof(wchar_t))
            return Fail();
        value.resize(length);
        return length == 0 || m_message.TryReadBytes(&value[0], length * sizeof(wchar_t));
    }

    bool ReadCount(int& value, int maximum) {
        if (!Read(value) || value < 0 || value > maximum)
            return Fail();
        return true;
    }

    bool Valid() const { return m_valid; }
    bool Finish(bool allowTrailingBytes) {
        if (!allowTrailingBytes && Remaining() != 0)
            return Fail();
        return m_valid;
    }

private:
    bool Fail() {
        m_valid = false;
        m_message.FlushRemaining();
        return false;
    }
    CMsgStreamBuffer& m_message;
    bool m_valid;
};
