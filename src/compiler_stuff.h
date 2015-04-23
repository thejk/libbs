#ifndef COMPILER_STUFF_H
#define COMPILER_STUFF_H

#if HAVE___ATTRIBUTE__
# define MALLOC __attribute__((malloc))
# define NONULL __attribute__((nonnull))
# define NONULL_ARGS(...) __attribute__((nonnull (__VA_ARGS__)))
# define PRINTF(_n,_m) __attribute__((format (printf, (_n), (_m))))
# define BS_API __attribute__((visibility=default))
#else
/**
 * Mark a function as always returning a new unique pointer or NULL
 */
# define MALLOC
/**
 * Mark a function as not taking NULL to any of its pointer parameters
 */
# define NONULL
/**
 * Mark a function as not taking NULL to the given list of pointer parameters.
 * The list of parameters is 1 based.
 */
# define NONULL_ARGS(...)
/**
 * Mark a function as taking printf format style parameter.
 * @param _n the 1 based index of the format parameter
 * @param _m the 1 based index of the first variable list parameter
 */
# define PRINTF(_n,_m)
/**
 * Mark a function as exported or visible
 */
# define BS_API
#endif

#endif /* COMPILER_STUFF_H */
