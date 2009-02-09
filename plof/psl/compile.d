/**
 * Compiler for ASCII PSL
 *
 * License:
 *  Copyright (c) 2007, 2008  Gregor Richards
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
import plof.psl.psl;

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
                ret ~= cast(ubyte) 0xFE;
            } else if (op[0] == '[') {
                ret ~= cast(ubyte) 0xFD;
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
    cast(char[])
    "push0"     :psl_push0,
    "push1"     :psl_push1,
    "push2"     :psl_push2,
    "push3"     :psl_push3,
    "push4"     :psl_push4,
    "push5"     :psl_push5,
    "push6"     :psl_push6,
    "push7"     :psl_push7,
    "pop"       :psl_pop,
    "this"      :psl_this,
    "null"      :psl_null,
    "global"    :psl_global,
    "new"       :psl_new,
    "combine"   :psl_combine,
    "member"    :psl_member,
    "memberset" :psl_memberset,
    "parent"    :psl_parent,
    "parentset" :psl_parentset,
    "call"      :psl_call,
    "return"    :psl_return,
    "throw"     :psl_throw,
    "catch"     :psl_catch,
    "cmp"       :psl_cmp,
    "concat"    :psl_concat,
    "wrap"      :psl_wrap,
    "resolve"   :psl_resolve,
    "calli"     :psl_calli,
    "loop"      :psl_loop,
    "replace"   :psl_replace,
    "array"     :psl_array,
    "aconcat"   :psl_aconcat,
    "length"    :psl_length,
    "lengthset" :psl_lengthset,
    "index"     :psl_index,
    "indexset"  :psl_indexset,
    "members"   :psl_members,
    "integer"   :psl_integer,
    "intwidth"  :psl_intwidth,
    "mul"       :psl_mul,
    "div"       :psl_div,
    "mod"       :psl_mod,
    "add"       :psl_add,
    "sub"       :psl_sub,
    "lt"        :psl_lt,
    "lte"       :psl_lte,
    "eq"        :psl_eq,
    "ne"        :psl_ne,
    "gt"        :psl_gt,
    "gte"       :psl_gte,
    "sl"        :psl_sl,
    "sr"        :psl_sr,
    "or"        :psl_or,
    "nor"       :psl_nor,
    "xor"       :psl_xor,
    "nxor"      :psl_nxor,
    "and"       :psl_and,
    "nand"      :psl_nand,
    "byte"      :psl_byte,
    "float"     :psl_float,
    "fint"      :psl_fint,
    "fmul"      :psl_fmul,
    "fdiv"      :psl_fdiv,
    "fmod"      :psl_fmod,
    "fadd"      :psl_fadd,
    "fsub"      :psl_fsub,
    "flt"       :psl_flt,
    "flte"      :psl_flte,
    "feq"       :psl_feq,
    "fne"       :psl_fne,
    "fgt"       :psl_fgt,
    "fgte"      :psl_fgte,
    "version"   :psl_version,
    "dsrcfile"  :psl_dsrcfile,
    "dsrcline"  :psl_dsrcline,
    "dsrccol"   :psl_dsrccol,
    "print"     :psl_print,
    "debug"     :psl_debug,
    "trap"      :psl_trap,
    "include"   :psl_include,
    "parse"     :psl_parse,
    "gadd"      :psl_gadd,
    "grem"      :psl_grem,
    "gaddstop"  :psl_gaddstop,
    "gremstop"  :psl_gremstop,
    "gaddgroup" :psl_gaddgroup,
    "gremgroup" :psl_gremgroup,
    "gcommit"   :psl_gcommit
    ];

    /* foreach (op; pslOps.keys.dup.sort) {
        Stdout("\"pslOp\" \"/")
              (op)
              ("/\" \"token\" \"white\" 3 integer array {{")
              (op)
              ("}} gadd").newline;
    } */
}
