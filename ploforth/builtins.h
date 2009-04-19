/*
 * Copyright (c) 2009 Gregor Richards
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

/* use this header to generate some code for every builtin */

#include "dict.h"

/* builtin words. In the form name, string name, flags */
FOREACH(nop, "nop", 0)
FOREACH(hello, "hello", 0)
FOREACH(define, ":", 0)
FOREACH(enddef, ";", DICT_FLAG_IMMEDIATE)
FOREACH(exit, "exit", 0)

#if 0
/******************************************************************************
 * these are fairly standard Forth
 *****************************************************************************/
FOREACH(drop, "drop", 0)
FOREACH(swap, "swap", 0)
FOREACH(dup, "dup", 0)
FOREACH(over, "over", 0)
FOREACH(rot, "rot", 0)
FOREACH(nrot, "-rot", 0)
FOREACH(_2drop, "2drop", 0)
FOREACH(_2dup, "2dup", 0)
FOREACH(_2swap, "2swap", 0)
FOREACH(qdup, "?dup", 0)
FOREACH(incr, "1+", 0)
FOREACH(decr, "1-", 0)
FOREACH(add, "+", 0)
FOREACH(sub, "-", 0)
FOREACH(mul, "*", 0)
FOREACH(div, "/", 0)
FOREACH(mod, "mod", 0)
FOREACH(eq, "=", 0)
FOREACH(ne, "<>", 0)
FOREACH(lt, "<", 0)
FOREACH(gt, ">", 0)
FOREACH(lte, "<=", 0)
FOREACH(gte, ">=", 0)
FOREACH(ze, "0=", 0)
FOREACH(nz, "0<>", 0)
FOREACH(and, "and", 0)
FOREACH(or, "or", 0)
FOREACH(xor, "xor", 0)
FOREACH(invert, "invert", 0)
FOREACH(lit, "lit", 0); /* push a literal in the argument */
FOREACH(litstr, "litstr", 0); /* push a literal string in the argument */

/******************************************************************************
 * these are for object-orientation
 *****************************************************************************/

/* standard objects and object creation */
FOREACH(global, "global", 0)
FOREACH(null, "null", 0)
FOREACH(new, "new", 0)

/* member access */
FOREACH(parent, "parent", 0)
FOREACH(parentset, "parentset", 0)
FOREACH(resolve, "resolve", 0)
FOREACH(member, "member", 0)
FOREACH(memberset, "memberset", 0)

/******************************************************************************
 * these are for procedure literals
 *****************************************************************************/
FOREACH(proc_start, "{", 0)
FOREACH(proc_end, "}", 0)
#endif
