/**
 * Constants for PSL operations
 *
 * License=
 *  Copyright (c) 2008  Gregor Richards
 *  
 *  Permission is hereby granted; free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"); to
 *  deal in the Software without restriction; including without limitation the
 *  rights to use; copy, modify, merge, publish, distribute, sublicense, and/or
 *  sell copies of the Software; and to permit persons to whom the Software is
 *  furnished to do so; subject to the following conditions=
 *  
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *  
 *  THE SOFTWARE IS PROVIDED "AS IS"; WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED; INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM; DAMAGES OR OTHER
 *  LIABILITY; WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM; OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *  IN THE SOFTWARE.
 */

module plof.psl.psl;

const ubyte psl_push0     = 0x00;
const ubyte psl_push1     = 0x01;
const ubyte psl_push2     = 0x02;
const ubyte psl_push3     = 0x03;
const ubyte psl_push4     = 0x04;
const ubyte psl_push5     = 0x05;
const ubyte psl_push6     = 0x06;
const ubyte psl_push7     = 0x07;
const ubyte psl_pop       = 0x08;
const ubyte psl_this      = 0x09;
const ubyte psl_null      = 0x0A;
const ubyte psl_global    = 0x0B;
const ubyte psl_new       = 0x0C;
const ubyte psl_combine   = 0x0D;
const ubyte psl_member    = 0x0E;
const ubyte psl_memberset = 0x0F;
const ubyte psl_parent    = 0x10;
const ubyte psl_resolve   = 0x11;
const ubyte psl_call      = 0x12;
const ubyte psl_return    = 0x13;
const ubyte psl_throw     = 0x14;
const ubyte psl_catch     = 0x15;
const ubyte psl_cmp       = 0x16;
const ubyte psl_concat    = 0x17;
const ubyte psl_wrap      = 0x18;
const ubyte psl_loop      = 0x1A;
const ubyte psl_array     = 0x20;
const ubyte psl_aconcat   = 0x21;
const ubyte psl_length    = 0x22;
const ubyte psl_lengthset = 0x23;
const ubyte psl_index     = 0x24;
const ubyte psl_indexset  = 0x25;
const ubyte psl_members   = 0x26;
const ubyte psl_integer   = 0x70;
const ubyte psl_intwidth  = 0x71;
const ubyte psl_mul       = 0x72;
const ubyte psl_div       = 0x73;
const ubyte psl_mod       = 0x74;
const ubyte psl_add       = 0x76;
const ubyte psl_sub       = 0x77;
const ubyte psl_lt        = 0x78;
const ubyte psl_lte       = 0x79;
const ubyte psl_eq        = 0x7A;
const ubyte psl_ne        = 0x7B;
const ubyte psl_gt        = 0x7C;
const ubyte psl_gte       = 0x7D;
const ubyte psl_sl        = 0x7E;
const ubyte psl_sr        = 0x7F;
const ubyte psl_or        = 0x80;
const ubyte psl_nor       = 0x81;
const ubyte psl_xor       = 0x82;
const ubyte psl_nxor      = 0x83;
const ubyte psl_and       = 0x84;
const ubyte psl_nand      = 0x85;
const ubyte psl_byte      = 0x8E;
const ubyte psl_float     = 0x90;
const ubyte psl_fint      = 0x91;
const ubyte psl_fmul      = 0x92;
const ubyte psl_fdiv      = 0x93;
const ubyte psl_fmod      = 0x94;
const ubyte psl_fadd      = 0x96;
const ubyte psl_fsub      = 0x97;
const ubyte psl_flt       = 0x98;
const ubyte psl_flte      = 0x99;
const ubyte psl_feq       = 0x9A;
const ubyte psl_fne       = 0x9B;
const ubyte psl_fgt       = 0x9C;
const ubyte psl_fgte      = 0x9D;
const ubyte psl_version   = 0xC0;
const ubyte psl_dsrcfile  = 0xD0;
const ubyte psl_dsrcline  = 0xD1;
const ubyte psl_dsrccol   = 0xD2;
const ubyte psl_print     = 0xE0;
const ubyte psl_debug     = 0xE1;
const ubyte psl_include   = 0xEE;
const ubyte psl_parse     = 0xEF;
const ubyte psl_gadd      = 0xF0;
const ubyte psl_grem      = 0xF1;
const ubyte psl_gaddstop  = 0xF2;
const ubyte psl_gremstop  = 0xF3;
const ubyte psl_gaddgroup = 0xF4;
const ubyte psl_gremgroup = 0xF5;
const ubyte psl_gcommit   = 0xFB;
const ubyte psl_marker    = 0xFC;
const ubyte psl_immediate = 0xFD;
const ubyte psl_code      = 0xFE;
const ubyte psl_raw       = 0xFF;

const ubyte psl_dlopen    = 0xC1;
const ubyte psl_dlclose   = 0xC2;
const ubyte psl_dlsym     = 0xC3;
const ubyte psl_cmalloc   = 0xC4;
const ubyte psl_cfree     = 0xC5;
const ubyte psl_cget      = 0xC6;
const ubyte psl_cset      = 0xC7;
const ubyte psl_ctype     = 0xC8;
const ubyte psl_cstruct   = 0xC9;
const ubyte psl_csizeof   = 0xCA;
const ubyte psl_csget     = 0xCB;
const ubyte psl_csset     = 0xCC;
const ubyte psl_prepcif   = 0xCD;
const ubyte psl_ccall     = 0xCE;
