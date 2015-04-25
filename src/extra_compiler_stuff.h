#ifndef EXTRA_COMPILER_STUFF_H
#define EXTRA_COMPILER_STUFF_H

#include "compiler_stuff.h"

#if HAVE___ATTRIBUTE__
# define UNUSED __attribute__((unused))
#else
/**
 * Mark a argument as unused
 */
# define UNUSED
#endif

#endif /* EXTRA_COMPILER_STUFF_H */
