#ifndef COMPILER_STUFF_H
#define COMPILER_STUFF_H

#if HAVE___ATTRIBUTE__
# define BS_MALLOC __attribute__((malloc))
# define BS_NONULL __attribute__((nonnull))
# define BS_NONULL_ARGS(...) __attribute__((nonnull (__VA_ARGS__)))
# define BS_API __attribute__((visibility("default")))
#else
/**
 * Mark a function as always returning a new unique pointer or NULL
 */
# define BS_MALLOC
/**
 * Mark a function as not taking NULL to any of its pointer parameters
 */
# define BS_NONULL
/**
 * Mark a function as not taking NULL to the given list of pointer parameters.
 * The list of parameters is 1 based.
 */
# define BS_NONULL_ARGS(...)
/**
 * Mark a function as exported or visible
 */
# define BS_API
#endif

#endif /* COMPILER_STUFF_H */
