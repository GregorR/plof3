#ifndef JUMP_H
#define JUMP_H

/* The macros defined by this file:
 * jumpenum:            Defined if enum jumplabel needs to be defined
 * jumphead:            Header needed before jump tables, if applicable
 * jumptail:            Tail needed after jump tables, if applicable
 * addressof(label):    Get the address of the given label
 * label(name):         Create a label with this name
 * prejump(var):        Prepare to jump. Use before jumphead, and will jump to
 *                      the right place at jumphead, or immediately
 *                      (implementation-dependent)
 * jump(var):           Jump to the specified address (in a variable)
 */


#if defined(__GNUC__) && !defined(FAKE_JUMPS)

/* The best real option */
#define jumpvars
#define jumphead
#define jumptail
#define addressof(label)        &&label
#define label(name)             name:
#define jump(var)               goto *(var)
#define prejump                 jump

#else /* if defined(FAKE_JUMPS) or unsupported compiler */

#define jumpenum                1
#define jumpvars                enum jumplabel __jump_to;
#define jumphead                while (1) { switch (__jump_to) {
#define jumptail                } }
#define addressof(label)        (void *) label
#define label(name)             case name:
#define prejump(var)            __jump_to = (enum jumplabel) (var)
#define jump(var)               __jump_to = (enum jumplabel) (var); break

#endif

#endif
