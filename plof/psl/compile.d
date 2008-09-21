/**
 * Compiler for ASCII PSL
 *
 * License:
 *  Copyright (c) 2007  Gregor Richards
 *  
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to
 *  deal in the Software without restriction, including without limitation the
 *  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 *  sell copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *  
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *  
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *  IN THE SOFTWARE.
 */

module plof.psl.compile;

import tango.io.Stdout;

import tango.text.convert.Integer;

import plof.psl.lex;

/** Compile PSL assembly into PSL binary data */
ubyte[] pslCompile(char[] asmtxt)
{
    ubyte[] ret;
    char[][] lexed = pslLex(asmtxt);

    foreach (op; lexed) {
        if (op[0] == '{' || op[0] == '[') {
            // a procedure or immediate
            ubyte[] proc = pslCompile(op[1..$-1]);
            
            if (op[0] == '{') {
                ret ~= cast(ubyte) 0xFF;
            } else if (op[0] == '[') {
                ret ~= cast(ubyte) 0xFE;
            }

            ret ~= pslBignum(proc.length) ~ proc;

        } else if (op[0] == '"') {
            // a string, slightly easier than a procedure
            ret ~= cast(ubyte[]) "\xFF" ~ pslBignum(op.length - 2) ~ cast(ubyte[]) op[1..$-1];

        } else if (op[0] >= '0' && op[0] <= '9') {
            // a number, push it as a 4-byte integer
            int val = toInt(op);

            ret ~= cast(ubyte[]) "\xFF\x04" ~
                   [cast(ubyte) (val >> 24),
                    cast(ubyte) ((val & 0xFF0000) >> 16),
                    cast(ubyte) ((val & 0xFF00) >> 8),
                    cast(ubyte) val];

        } else {
            // otherwise, just compile it
            ubyte* oop = op in pslOps;
            if (oop is null) {
                Stderr("Unrecognized operation ")(op).newline;
            } else {
                ret ~= pslOps[op];
            }
        }
    }

    return ret;
}

/** Make a PSL-style bignum */
ubyte[] pslBignum(ulong n)
{
    if (n < 128) {
        return [cast(ubyte) n];
    }

    ubyte[] prev = pslBignum(n >> 7);
    prev[$-1] = prev[$-1] ^ 0x80;
    return (prev ~ (cast(ubyte) (n % 128)));
}

/** All of the PSL ops, as a handy associative array */
ubyte[char[]] pslOps;
static this() {
    pslOps = [
    cast(char[]) "push0"     : 0x00,
    "push1"     : 0x01,
    "push2"     : 0x02,
    "push3"     : 0x03,
    "push4"     : 0x04,
    "push5"     : 0x05,
    "push6"     : 0x06,
    "push7"     : 0x07,
    "pop"       : 0x08,
    "this"      : 0x09,
    "null"      : 0x0A,
    "global"    : 0x0B,
    "new"       : 0x0C,
    "combine"   : 0x0D,
    "member"    : 0x0E,
    "memberset" : 0x0F,
    "parent"    : 0x10,
    "resolve"   : 0x11,
    "call"      : 0x12,
    "return"    : 0x13,
    "throw"     : 0x14,
    "catch"     : 0x15,
    "cmp"       : 0x16,
    "concat"    : 0x17,
    "wrap"      : 0x18,
    "iwrap"     : 0x19,
    "loop"      : 0x1A,
    "array"     : 0x20,
    "aconcat"   : 0x21,
    "length"    : 0x22,
    "lengthset" : 0x23,
    "index"     : 0x24,
    "indexset"  : 0x25,
    "members"   : 0x26,
    "integer"   : 0x70,
    "intwidth"  : 0x71,
    "mul"       : 0x72,
    "div"       : 0x73,
    "mod"       : 0x74,
    "add"       : 0x76,
    "sub"       : 0x77,
    "lt"        : 0x78,
    "lte"       : 0x79,
    "eq"        : 0x7A,
    "ne"        : 0x7B,
    "gt"        : 0x7C,
    "gte"       : 0x7D,
    "sl"        : 0x7E,
    "sr"        : 0x7F,
    "or"        : 0x80,
    "nor"       : 0x81,
    "xor"       : 0x82,
    "nxor"      : 0x83,
    "and"       : 0x84,
    "nand"      : 0x85,
    "byte"      : 0x8E,
    "float"     : 0x90,
    "fint"      : 0x91,
    "fmul"      : 0x92,
    "fdiv"      : 0x93,
    "fmod"      : 0x94,
    "fadd"      : 0x96,
    "fsub"      : 0x97,
    "flt"       : 0x98,
    "flte"      : 0x99,
    "feq"       : 0x9A,
    "fne"       : 0x9B,
    "fgt"       : 0x9C,
    "fgte"      : 0x9D,
    "version"   : 0xC0,
    "dsrcfile"  : 0xD0,
    "dsrcline"  : 0xD1,
    "dsrccol"   : 0xD2,
    "print"     : 0xE0,
    "debug"     : 0xE1,
    "include"   : 0xEE,
    "parse"     : 0xEF,
    "gadd"      : 0xF0,
    "grem"      : 0xF1,
    "gaddstop"  : 0xF2,
    "gremstop"  : 0xF3,
    "gaddgroup" : 0xF4,
    "gremgroup" : 0xF5,
    "gcommit"   : 0xFD
    ];

    /* foreach (op; pslOps.keys.dup.sort) {
        Stdout("\"pslOp\" \"/")
              (op)
              ("/\" \"token\" \"white\" 3 integer array {{")
              (op)
              ("}} gadd").newline;
    } */
}
