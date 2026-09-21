#pragma once


#define TID_ARG(value, offset) \
    ((value) << (offset))

#define TID_MASK(offset, size) \
    (TID_ARG((((tid_t) -1) >> ((sizeof(tid_t) * 8) - (size))), (offset)))

typedef unsigned short tid_t;

template<tid_t TID1, tid_t TID2, tid_t TID3, tid_t TID4>
class TID {
};

template<tid_t TID1, tid_t TID2>
class TID_2 {
};

template<tid_t TID1, tid_t TID2, tid_t TID3>
class TID_3 {
};


class TypeId {
    enum {
        TID0_OFFSET = 1,
        TID0_SIZE = 1,
        TID0_MASK = TID_MASK(TID0_OFFSET, TID0_SIZE),

        TID1_OFFSET = 2,
        TID1_SIZE = 3,
        TID1_MASK = TID_MASK(TID1_OFFSET, TID1_SIZE),

        TID2_OFFSET = 5,
        TID2_SIZE = 2,
        TID2_MASK = TID_MASK(TID2_OFFSET, TID2_SIZE),

        TID3_OFFSET = 7,
        TID3_SIZE = 4,
        TID3_MASK = TID_MASK(TID3_OFFSET, TID3_SIZE),

        TID4_OFFSET = 11,
        TID4_SIZE = 5,
        TID4_MASK = TID_MASK(TID4_OFFSET, TID4_SIZE),
    };

public:


    tid_t getTypeID1() const;
    tid_t getTypeID2() const;
    tid_t getTypeID3() const;
    tid_t getTypeID4() const;

    explicit TypeId(unsigned short tid) : m_type_id_value(tid) {}

    template<unsigned short TID1, unsigned short TID2, unsigned short TID3, unsigned short TID4>
    bool Is(TID<TID1, TID2, TID3, TID4> tid) const {
        return ((m_type_id_value & TID1_MASK) == TID_ARG(TID1, TID1_OFFSET)) &&
               ((m_type_id_value & TID2_MASK) == TID_ARG(TID2, TID2_OFFSET)) &&
               ((m_type_id_value & TID3_MASK) == TID_ARG(TID3, TID3_OFFSET)) &&
               ((m_type_id_value & TID4_MASK) == TID_ARG(TID4, TID4_OFFSET));
    }

    template<unsigned short TID1, unsigned short TID2>
    bool Is(TID_2<TID1, TID2> tid) const {
        return ((m_type_id_value & TID1_MASK) == TID_ARG(TID1, TID1_OFFSET)) &&
               ((m_type_id_value & TID2_MASK) == TID_ARG(TID2, TID2_OFFSET));
    }

    template<unsigned short TID1, unsigned short TID2, unsigned short TID3>
    bool Is(TID_3<TID1, TID2, TID3> tid) const {
        return ((m_type_id_value & TID1_MASK) == TID_ARG(TID1, TID1_OFFSET)) &&
               ((m_type_id_value & TID2_MASK) == TID_ARG(TID2, TID2_OFFSET)) &&
               ((m_type_id_value & TID3_MASK) == TID_ARG(TID3, TID3_OFFSET));
    }

public:
    unsigned short m_type_id_value;
};
