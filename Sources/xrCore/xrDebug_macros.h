#pragma once

#define DEBUG_INFO __FILE__, __LINE__, __FUNCTION__

// ---------------------------------------------------------------------------
// R_ASSERT Ц проверка услови€ (без дополнительных сообщений)
// ---------------------------------------------------------------------------
#define R_ASSERT(expr)                                                                   \
    do {                                                                                 \
        static bool ignore_always = false;                                               \
        if (!ignore_always && !(expr)) {                                                 \
            ::Debug.fail(#expr, DEBUG_INFO, ignore_always);                              \
        }                                                                                \
    } while(0)

// — дополнительными сообщени€ми
#define R_ASSERT2(expr, e2)                                                              \
    do {                                                                                 \
        static bool ignore_always = false;                                               \
        if (!ignore_always && !(expr)) {                                                 \
            ::Debug.fail(#expr, e2, DEBUG_INFO, ignore_always);                          \
        }                                                                                \
    } while(0)

#define R_ASSERT3(expr, e2, e3)                                                          \
    do {                                                                                 \
        static bool ignore_always = false;                                               \
        if (!ignore_always && !(expr)) {                                                 \
            ::Debug.fail(#expr, e2, e3, DEBUG_INFO, ignore_always);                      \
        }                                                                                \
    } while(0)

#define R_ASSERT4(expr, e2, e3, e4)                                                      \
    do {                                                                                 \
        static bool ignore_always = false;                                               \
        if (!ignore_always && !(expr)) {                                                 \
            ::Debug.fail(#expr, e2, e3, e4, DEBUG_INFO, ignore_always);                  \
        }                                                                                \
    } while(0)

// ---------------------------------------------------------------------------
// R_CHK Ц проверка HRESULT
// ---------------------------------------------------------------------------
#define R_CHK(expr)                                                                      \
    do {                                                                                 \
        static bool ignore_always = false;                                               \
        HRESULT hr = expr;                                                               \
        if (!ignore_always && FAILED(hr)) {                                              \
            ::Debug.error(hr, #expr, DEBUG_INFO, ignore_always);                         \
        }                                                                                \
    } while(0)

#define R_CHK2(expr, e2)                                                                 \
    do {                                                                                 \
        static bool ignore_always = false;                                               \
        HRESULT hr = expr;                                                               \
        if (!ignore_always && FAILED(hr)) {                                              \
            ::Debug.error(hr, #expr, e2, DEBUG_INFO, ignore_always);                     \
        }                                                                                \
    } while(0)

// ---------------------------------------------------------------------------
// FATAL Ц фатальна€ ошибка
// ---------------------------------------------------------------------------
#define FATAL(description)  ::Debug.fatal(DEBUG_INFO, description)

// ---------------------------------------------------------------------------
// CHECK_OR_EXIT Ц немедленное завершение
// ---------------------------------------------------------------------------
#define CHECK_OR_EXIT(expr, message)                                                     \
    do {                                                                                 \
        if (!(expr))                                                                     \
            ::Debug.do_exit(message);                                                    \
    } while(0)

#ifdef VERIFY
#undef VERIFY
#endif

#ifdef DEBUG
#define NODEFAULT  FATAL("nodefault reached")

// VERIFY Ц проверка в отладочной сборке
#define VERIFY(expr)                                                                 \
        do {                                                                             \
            static bool ignore_always = false;                                           \
            if (!ignore_always && !(expr)) {                                             \
                ::Debug.fail(#expr, DEBUG_INFO, ignore_always);                          \
            }                                                                            \
        } while(0)

#define VERIFY2(expr, e2)                                                            \
        do {                                                                             \
            static bool ignore_always = false;                                           \
            if (!ignore_always && !(expr)) {                                             \
                ::Debug.fail(#expr, e2, DEBUG_INFO, ignore_always);                      \
            }                                                                            \
        } while(0)

#define VERIFY3(expr, e2, e3)                                                        \
        do {                                                                             \
            static bool ignore_always = false;                                           \
            if (!ignore_always && !(expr)) {                                             \
                ::Debug.fail(#expr, e2, e3, DEBUG_INFO, ignore_always);                  \
            }                                                                            \
        } while(0)

#define VERIFY4(expr, e2, e3, e4)                                                    \
        do {                                                                             \
            static bool ignore_always = false;                                           \
            if (!ignore_always && !(expr)) {                                             \
                ::Debug.fail(#expr, e2, e3, e4, DEBUG_INFO, ignore_always);              \
            }                                                                            \
        } while(0)

#define CHK_DX(expr)                                                                 \
        do {                                                                             \
            static bool ignore_always = false;                                           \
            HRESULT hr = expr;                                                           \
            if (!ignore_always && FAILED(hr))                                             \
                ::Debug.error(hr, #expr, DEBUG_INFO, ignore_always);                     \
        } while(0)

#else // RELEASE

#define NODEFAULT  __assume(0)

#define VERIFY(expr)
#define VERIFY2(expr, e2)
#define VERIFY3(expr, e2, e3)
#define VERIFY4(expr, e2, e3, e4)

#define CHK_DX(expr)  ((void)(expr))

#endif // DEBUG

// ---------------------------------------------------------------------------
// FIXME / TODO / NOTE
// ---------------------------------------------------------------------------
#define _QUOTE(x)       #x
#define QUOTE(x)        _QUOTE(x)
#define __FILE__LINE__  __FILE__ "(" QUOTE(__LINE__) ") : "

#define NOTE(x)         __pragma(message(x))
#define FILE_LINE       __pragma(message(__FILE__LINE__))

#define TODO(x)         __pragma(message(__FILE__LINE__ "\n"                        \
                         " ------------------------------------------------\n"      \
                         "|  TODO :   " #x "\n"                                     \
                         " -------------------------------------------------\n"))
#define FIXME(x)        __pragma(message(__FILE__LINE__ "\n"                        \
                         " ------------------------------------------------\n"      \
                         "|  FIXME :  " #x "\n"                                     \
                         " -------------------------------------------------\n"))
#define todo(x)         __pragma(message(__FILE__LINE__ " TODO :   " #x "\n"))
#define fixme(x)        __pragma(message(__FILE__LINE__ " FIXME:   " #x "\n"))

// ---------------------------------------------------------------------------
// Compile-time assertion
// ---------------------------------------------------------------------------
template <bool> struct CompileTimeError;
template <> struct CompileTimeError<true> {};
#define STATIC_CHECK(expr, msg)                                         \
    {                                                                   \
        CompileTimeError<((expr) != 0)> ERROR_##msg;                    \
        (void)ERROR_##msg;                                              \
    }
