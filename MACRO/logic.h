#define PRIMITIVE_CAT(a, b) a##b
#define CAT(a, b) PRIMITIVE_CAT(a, b)

#define EMPTY_IF_PAREN(x) EAT x
#define EAT(...)

#define IF(x) CAT(__IF_, x)
#define __IF_1(...) __VA_ARGS__ EAT
#define __IF_0(...) SELF
#define SELF(...) __VA_ARGS__

#define GET_SEC(x, n, ...) n
#define CHECK(...) GET_SEC(__VA_ARGS__, 0)
#define PROBE(x) x, 1

#define IS_EMPTY(x) IS_EMPTY_(x)
#define IS_EMPTY_(x) CHECK(PROBE_##x)
#define PROBE_ PROBE()

#define NOT(x) IF(x)(0)(1)
#define AND(x, y) IF(x)(IF(y)(1)(0))(0)
#define OR(x, y) IF(x)(1)(IF(y)(1)(0))

#define BOOL(x) BOOL_(x) // only 0 or empty is false
#define BOOL_(x) CHECK(BOOL_PROBE_##x)
#define BOOL_PROBE_ PROBE()
#define BOOL_PROBE_0 PROBE()

#define EMPTY()
#define DEFER(id) id EMPTY()
#define EVAL(...) __VA_ARGS__

#define __FOREACH(macro, x, ...) macro(x) DEFER(__FOREACH_)(NOT(HAS_ARG(__VA_ARGS__)))(macro, __VA_ARGS__)
#define __FOREACH_(x) IF(x)(EAT)(__FOREACH)
#define IS_EMPTY_0(x, ...) IS_EMPTY_(x)
#define EVAL1(...) EVAL(EVAL(EVAL(__VA_ARGS__)))
#define EVAL2(...) EVAL1(EVAL1(EVAL1(__VA_ARGS__)))
#define EVAL3(...) EVAL2(EVAL2(EVAL2(__VA_ARGS__)))
#define EVAL4(...) EVAL3(EVAL3(EVAL3(__VA_ARGS__)))
#ifdef SHORT_EVAL
#define EVAL5 EVAL2
#else
#define EVAL5(...) EVAL4(EVAL4(EVAL4(__VA_ARGS__)))
#endif
#define FOREACH(macro, ...) EVAL5(__FOREACH(macro, __VA_ARGS__))

#define __FOREACH_DOUBLE(macro, x, y, ...) macro(x, y) DEFER(__FOREACH_DOUBLE_)(NOT(HAS_ARG(__VA_ARGS__)))(macro, __VA_ARGS__)
#define __FOREACH_DOUBLE_(x) IF(x)(EAT)(__FOREACH_DOUBLE)
#define FOREACH_DOUBLE(macro, ...) EVAL5(__FOREACH_DOUBLE(macro, __VA_ARGS__))

#define __FOREACH_WITH_PREFIX(macro, prefix, x, ...) \
    macro(prefix, x) DEFER(__FOREACH_WITH_PREFIX_)(prefix, NOT(HAS_ARG(__VA_ARGS__)))(macro, prefix, __VA_ARGS__)
#define __FOREACH_WITH_PREFIX_(prefix, x) IF(x)(EAT)(__FOREACH_WITH_PREFIX)
#define FOREACH_WITH_PREFIX(macro, prefix, ...) EVAL5(__FOREACH_WITH_PREFIX(macro, prefix, __VA_ARGS__))

#define __FOREACH_DOUBLE_WITH_PREFIX(macro, prefix, x, y, ...) macro(prefix, x, y) \
    DEFER(__FOREACH_DOUBLE_WITH_PREFIX_)(prefix, NOT(HAS_ARG(__VA_ARGS__)))(macro, prefix, __VA_ARGS__)
#define __FOREACH_DOUBLE_WITH_PREFIX_(prefix, x) IF(x)(EAT)(__FOREACH_DOUBLE_WITH_PREFIX)
#define FOREACH_DOUBLE_WITH_PREFIX(macro, prefix, ...) EVAL5(__FOREACH_DOUBLE_WITH_PREFIX(macro, prefix, __VA_ARGS__))


#define __FOREACH_WITH_DOUBLE_PREFIX(macro, prefix1, prefix2, x, ...) macro(prefix1, prefix2, x) \
    DEFER(__FOREACH_WITH_DOUBLE_PREFIX_)(prefix1, prefix2, NOT(HAS_ARG(__VA_ARGS__)))(macro, prefix1, prefix2, __VA_ARGS__)
#define __FOREACH_WITH_DOUBLE_PREFIX_(prefix1, prefix2, x) IF(x)(EAT)(__FOREACH_WITH_DOUBLE_PREFIX)
#define FOREACH_WITH_DOUBLE_PREFIX(macro, prefix1, prefix2, ...) EVAL5(__FOREACH_WITH_DOUBLE_PREFIX(macro, prefix1, prefix2, __VA_ARGS__))

#define __FOREACH_WITH_TRIPLE_PREFIX(macro, prefix1, prefix2, prefix3, x, ...) macro(prefix1, prefix2, prefix3, x) \
    DEFER(__FOREACH_WITH_TRIPLE_PREFIX_)(prefix1, prefix2, prefix3, NOT(HAS_ARG(__VA_ARGS__)))(macro, prefix1, prefix2, prefix3, __VA_ARGS__)
#define __FOREACH_WITH_TRIPLE_PREFIX_(prefix1, prefix2, prefix3, x) IF(x)(EAT)(__FOREACH_WITH_TRIPLE_PREFIX)
#define FOREACH_WITH_TRIPLE_PREFIX(macro, prefix1, prefix2, prefix3, ...) EVAL5(__FOREACH_WITH_TRIPLE_PREFIX(macro, prefix1, prefix2, prefix3, __VA_ARGS__))

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define COUNT_ARGS(...) COUNT_ARGS_IMPL(0, ##__VA_ARGS__, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define COUNT_ARGS_IMPL(_65, _64, _63, _62, _61, _60, _59, _58, _57, _56, _55, _54, _53, _52, _51, _50, _49, _48, _47, _46, _45, _44, _43, _42, _41, _40, _39, _38, _37, _36, _35, _34, _33, _32, _31, _30, _29, _28, _27, _26, _25, _24, _23, _22, _21, _20, _19, _18, _17, _16, _15, _14, _13, _12, _11, _10, _9, _8, _7, _6, _5, _4, _3, _2, _1, N, ...) N

#define HAS_ARG(...) __HAS_ARG(0, ##__VA_ARGS__, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0)
#define __HAS_ARG(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, N, ...) N
