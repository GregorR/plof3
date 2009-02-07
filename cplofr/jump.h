#ifndef JUMP_H
#define JUMP_H

/* The macros defined by this file:
 * addressof(label):    Get the address of the given label
 * label(name):         Create a label with this name
 * jump(var):           Jump to the specified address (in a variable)
 */

#ifdef __GNUC__

/* The best and simplest option */
#define addressof(label)        &&label
#define label(name)             name:
#define jump(var)               goto *(var)

#endif

#endif
