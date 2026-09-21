#include "TypeId.h"
tid_t TypeId::getTypeID1() const
{
    return (m_type_id_value & TID1_MASK)/4;
}
tid_t TypeId::getTypeID2() const
{
    return (m_type_id_value & TID2_MASK)/32;
}
tid_t TypeId::getTypeID3() const
{
    return (m_type_id_value & TID3_MASK)/128;
}
tid_t TypeId::getTypeID4() const
{
    return (m_type_id_value & TID4_MASK)/2048;
}