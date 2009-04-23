/*
 * Copyright (c) 2007, 2008, 2009 Gregor Richards
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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
