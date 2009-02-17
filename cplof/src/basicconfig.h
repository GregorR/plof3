#ifndef BASICCONFIG_H
#define BASICCONFIG_H

/* Try to guess some of the things that configure would have otherwise found */

#if defined(unix) || defined(__unix__) || defined(__unix)
#define HAVE_UNISTD_H 1
#endif

#define SIZEOF_VOID_P 4

#endif
