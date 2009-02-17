#ifndef BASICCONFIG_H
#define BASICCONFIG_H

/* Try to guess some of the things that configure would have otherwise found */

/* Choose a good method of boxing */
#if !defined(PLOF_BOX_NUMBERS) && !defined(PLOF_NUMBERS_IN_OBJECTS) && !defined(PLOF_FREE_INTS)
#define PLOF_FREE_INTS
#endif

/* We have unistd.h if we're on UNIX probably */
#if defined(unix) || defined(__unix__) || defined(__unix)
#define HAVE_UNISTD_H 1
#endif

/* Assume 32-bit */
#ifndef SIZEOF_VOID_P
#define SIZEOF_VOID_P 4
#endif

#endif
