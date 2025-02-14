#ifndef __VA_FUNC_DEF_H__
#define __VA_FUNC_DEF_H__


#include <stdarg.h>
#include "../../../MACRO/macro.h"

#define VA_CALL(func, ...) func(COUNT_ARGS(__VA_ARGS__), ##__VA_ARGS__)
#define ARG(type, name) name = va_arg(__VA_ARGS_INNER__, type);
// #define ARG_WITH_VA_IN(type, name) name = va_arg(__VA_LIST_IN__, type);
#define IDENTITY(x) x
#define STRUCT_SAFE(type, name) type name;
#define ARG_SAFE(type, name) __VA_DATA__->name = va_arg(__VA_ARGS_INNER__, type);
#define SAVE_ARG(type, name) name = __VA_DATA__->name;

/**
 * @brief This macro is used to define a function with fixed signiture but takes parameters as normal
 * 
 * @param type1 type of the first parameter
 * @param name1 name of the first parameter
 * @param ... the rest of the parameters
 * 
 * @code
 * ```c
 * char* func VA_DEF_FUNC(int, x, double, y) {
 *      printf("x = %d, y = %f\n", x, y);
 *      if (x > 0) VA_RETURN("x is positive");
 * VA_END_FUNC("Example Return");
 * ```
 * @endcode
 */
#define VA_DEF_FUNC(...) (va_list __VA_LIST_IN__, int __VA_ARGN_INNER__, ...) { \
    va_list __VA_ARGS_INNER__; \
    if (__VA_ARGN_INNER__ == -2) { \
        va_copy(__VA_ARGS_INNER__, __VA_LIST_IN__); \
    } else { \
        va_start(__VA_ARGS_INNER__, __VA_ARGN_INNER__); \
    } \
    FOREACH_DOUBLE(STRUCT_SAFE, __VA_ARGS__); \
    struct {FOREACH_DOUBLE(STRUCT_SAFE, __VA_ARGS__)}* __VA_DATA__ = NULL; \
    if (__VA_ARGN_INNER__ == -1) { \
        __VA_DATA__ = va_arg(__VA_ARGS_INNER__, void*); \
        FOREACH_DOUBLE(SAVE_ARG, __VA_ARGS__); \
    } else { \
        FOREACH_DOUBLE(ARG, __VA_ARGS__); \
    };
#define VA_DEF_ONLY(...) (va_list, int, ...)
#define VA_RETURN(ret_val) do {va_end(__VA_ARGS_INNER__); return ret_val; } while(0);
#define VA_END_FUNC(ret_val) VA_RETURN(ret_val);};
#define DEFINE_AND_SAVE_ARG(type, name) type name = __VA_DATA__->name;


// Note: release __VA_DATA__
/**
 * @brief This macro is similar to VA_DEF_FUNC but:
 *      it saves all the parameters to a struct
 * 
 * @param type1 type of the first parameter
 * @param name1 name of the first parameter
 * @param ... the rest of the parameters
 * 
 * @code
 * ```c
 * char* func VA_DEF_FUNC_SAFE(int, x, double, y) {
 *      printf("x = %d, y = %f\n", x, y);
 *      if (x > 0) VA_RETURN_SAFE("x is positive");
 * VA_END_FUNC_SAFE("Example Return");
 * ```
 * @endcode
 */
#define VA_DEF_FUNC_SAFE(error_ret, ...) (va_list __VA_LIST_IN__, int __VA_ARGN_INNER__, ...) { \
    va_list __VA_ARGS_INNER__; \
    if (__VA_ARGN_INNER__ == -2) { \
        va_copy(__VA_ARGS_INNER__, __VA_LIST_IN__); \
    } else { \
        va_start(__VA_ARGS_INNER__, __VA_ARGN_INNER__); \
    } \
    struct {FOREACH_DOUBLE(STRUCT_SAFE, __VA_ARGS__)}* __VA_DATA__; \
    if (__VA_ARGN_INNER__ == -1) { \
        __VA_DATA__ = va_arg(__VA_ARGS_INNER__, void*); \
    } else { \
        __VA_DATA__ = malloc(sizeof(struct {FOREACH_DOUBLE(STRUCT_SAFE, __VA_ARGS__)})); \
        if (__VA_DATA__ == NULL) { \
            va_end(__VA_ARGS_INNER__); \
            MemoryError("VA_FUNC_SAFE", "Failed to allocate memory for __VA_DATA__"); \
            return error_ret; \
        } \
        FOREACH_DOUBLE(ARG_SAFE, __VA_ARGS__); \
    } \
    FOREACH_DOUBLE(DEFINE_AND_SAVE_ARG, __VA_ARGS__);
#define VA_RETURN_SAFE(ret_val) do {free(__VA_DATA__); va_end(__VA_ARGS_INNER__); return ret_val; } while(0);
#define VA_END_FUNC_SAFE(ret_val) free(__VA_DATA__); VA_END_FUNC(ret_val); 

// VA_CALL_WITH_VADATA(f, vadata=__VA_DATA__)
#define VA_CALL_WITH_VADATA(f, ...) IF(IS_EMPTY(__VA_ARGS__))(f(VA_NULL, -1, __VA_DATA__))(f(VA_NULL, -1, __VA_ARGS__))

#define VA_CALL_WITH_VALIST(f, valist) f(valist, -2)

#define VA_DATA_STRUCT(...) struct {FOREACH_DOUBLE(STRUCT_SAFE, __VA_ARGS__)}


extern va_list VA_NULL;

#endif