#pragma once

#include <cstddef>
#include <cstdlib>

#define STRING(s) #s


/// \brief Ensures that given member is at given offset.
/// \param parameter Member to check
/// \param offset Offset the member should be at
#define ENSURE_OFFSET(parameter, offset)                   \
    class parameter {};                                    \
    enum { __offset##parameter = offsetof(T, parameter) }; \
    template<typename Member, int Want, int Have>          \
    class CheckHelper##parameter {                         \
        char checker[Want == Have ? 1 : -1];               \
    };                                                     \
    CheckHelper##parameter<parameter, offset, __offset##parameter> __check##parameter;


/// \brief Ensure that class is at given size in bytes
/// \param size Expected size of class in bytes
#define ENSURE_SIZE(size) \
    int __sizeof[sizeof(T) == size ? 1 : -1];


#define BEGIN_FIXTURE()  \
private:                 \
    template<typename T> \
    class TestFixture {


#define END_FIXTURE() \
    };


#define RUN_FIXTURE(cls) \
    static void _RunFixture() { static cls ::TestFixture<cls> testFixture; }


#define check_size(class_name, sz) \
    check_size_tpl<class_name, sz>(STRING(class_name))

template<typename T, std::size_t expected_size>
bool check_size_tpl(const char *class_name) {
    bool size_ok = sizeof(T) == expected_size;
    printf("[%s] sizeof(%s) 0x%X %s 0x%X\n", size_ok ? "OK" : "FAIL", class_name, sizeof(T), size_ok ? "==" : "!=",
           expected_size);
    return size_ok;
}
