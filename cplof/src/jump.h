/*
 * Copyright (c) 2007, 2008, 2009 gregor richards
 * 
 * permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "software"), to deal
 * in the software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the software, and to permit persons to whom the software is
 * furnished to do so, subject to the following conditions:
 * 
 * the above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the software.
 * 
 * the software is provided "as is", without warranty of any kind, express or
 * implied, including but not limited to the warranties of merchantability,
 * fitness for a particular purpose and noninfringement. in no event shall the
 * authors or copyright holders be liable for any claim, damages or other
 * liability, whether in an action of contract, tort or otherwise, arising from,
 * out of or in connection with the software or the use or other dealings in
 * the software.
 */

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
#define jump(var)               goto *((void *) (var))
#define prejump                 jump

#else /* if defined(FAKE_JUMPS) or unsupported compiler */

#define jumpenum                1
#define jumpvars                enum jumplabel __jump_to;
#define jumphead                while (1) { switch (__jump_to) {
#define jumptail                } }
#define addressof(label)        (void *) (size_t) label
#define label(name)             case name:
#define prejump(var)            __jump_to = (enum jumplabel) (size_t) (var)
#define jump(var)               __jump_to = (enum jumplabel) (size_t) (var); break

#endif

#endif
