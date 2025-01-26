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

#define __FOREACH(macro, x, ...) macro(x) DEFER(__FOREACH_)(IS_EMPTY_0(__VA_ARGS__))(macro, __VA_ARGS__)
#define __FOREACH_(x) IF(x)(EAT)(__FOREACH)
#define IS_EMPTY_0(x, ...) IS_EMPTY_(x)
#define EVAL1(...) EVAL(EVAL(EVAL(__VA_ARGS__)))
#define EVAL2(...) EVAL1(EVAL1(EVAL1(__VA_ARGS__)))
#define EVAL3(...) EVAL2(EVAL2(EVAL2(__VA_ARGS__)))
#define EVAL4(...) EVAL3(EVAL3(EVAL3(__VA_ARGS__)))
#define EVAL5(...) EVAL4(EVAL4(EVAL4(__VA_ARGS__)))
#define FOREACH(macro, ...) EVAL5(__FOREACH(macro, __VA_ARGS__))

#define __FOREACH_WITH_PREFIX(macro, prefix, x, ...) \
    macro(prefix, x) DEFER(__FOREACH_WITH_PREFIX_)(prefix, IS_EMPTY_0(__VA_ARGS__))(macro, prefix, __VA_ARGS__)
#define __FOREACH_WITH_PREFIX_(prefix, x) IF(x)(EAT)(__FOREACH_WITH_PREFIX)
#define FOREACH_WITH_PREFIX(macro, prefix, ...) EVAL5(__FOREACH_WITH_PREFIX(macro, prefix, __VA_ARGS__))

#define __FOREACH_WITH_DOUBLE_PREFIX(macro, prefix1, prefix2, x, ...) macro(prefix1, prefix2, x) \
    DEFER(__FOREACH_WITH_DOUBLE_PREFIX_)(prefix1, prefix2, IS_EMPTY_0(__VA_ARGS__))(macro, prefix1, prefix2, __VA_ARGS__)
#define __FOREACH_WITH_DOUBLE_PREFIX_(prefix1, prefix2, x) IF(x)(EAT)(__FOREACH_WITH_DOUBLE_PREFIX)
#define FOREACH_WITH_DOUBLE_PREFIX(macro, prefix1, prefix2, ...) EVAL5(__FOREACH_WITH_DOUBLE_PREFIX(macro, prefix1, prefix2, __VA_ARGS__))

#define __FOREACH_WITH_TRIPLE_PREFIX(macro, prefix1, prefix2, prefix3, x, ...) macro(prefix1, prefix2, prefix3, x) \
    DEFER(__FOREACH_WITH_TRIPLE_PREFIX_)(prefix1, prefix2, prefix3, IS_EMPTY_0(__VA_ARGS__))(macro, prefix1, prefix2, prefix3, __VA_ARGS__)
#define __FOREACH_WITH_TRIPLE_PREFIX_(prefix1, prefix2, prefix3, x) IF(x)(EAT)(__FOREACH_WITH_TRIPLE_PREFIX)
#define FOREACH_WITH_TRIPLE_PREFIX(macro, prefix1, prefix2, prefix3, ...) EVAL5(__FOREACH_WITH_TRIPLE_PREFIX(macro, prefix1, prefix2, prefix3, __VA_ARGS__))

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))