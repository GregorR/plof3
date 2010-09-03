/*
 * The master list of PSL instructions with associated bytecode, as well as
 * special names in PSL
 *
 * Copyright (C) 2009 Gregor Richards
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

#ifndef PLOF_PSL_H
#define PLOF_PSL_H

#define psl_push0     0x00
#define psl_push1     0x01
#define psl_push2     0x02
#define psl_push3     0x03
#define psl_push4     0x04
#define psl_push5     0x05
#define psl_push6     0x06
#define psl_push7     0x07
#define psl_pop       0x08
#define psl_this      0x09
#define psl_null      0x0A
#define psl_global    0x0B
#define psl_new       0x0C
#define psl_combine   0x0D
#define psl_member    0x0E
#define psl_memberset 0x0F
#define psl_parent    0x10
#define psl_parentset 0x11
#define psl_call      0x12
#define psl_return    0x13
#define psl_throw     0x14
#define psl_catch     0x15
#define psl_cmp       0x16
#define psl_concat    0x17
#define psl_wrap      0x18
#define psl_resolve   0x19
#define psl_while     0x1A
#define psl_calli     0x1B
#define psl_replace   0x1C
#define psl_array     0x20
#define psl_aconcat   0x21
#define psl_length    0x22
#define psl_lengthset 0x23
#define psl_index     0x24
#define psl_indexset  0x25
#define psl_members   0x26
#define psl_locals    0x27
#define psl_local     0x28
#define psl_localset  0x29
#define psl_nullarray 0x2A
#define psl_rawlength 0x60
#define psl_slice     0x61
#define psl_rawcmp    0x62
#define psl_extractraw 0x63
#define psl_integer   0x70
#define psl_intwidth  0x71
#define psl_mul       0x72
#define psl_div       0x73
#define psl_mod       0x74
#define psl_add       0x76
#define psl_sub       0x77
#define psl_lt        0x78
#define psl_lte       0x79
#define psl_eq        0x7A
#define psl_ne        0x7B
#define psl_gt        0x7C
#define psl_gte       0x7D
#define psl_sl        0x7E
#define psl_sr        0x7F
#define psl_or        0x80
#define psl_nor       0x81
#define psl_xor       0x82
#define psl_nxor      0x83
#define psl_and       0x84
#define psl_nand      0x85
#define psl_byte      0x8E
#define psl_float     0x90
#define psl_fint      0x91
#define psl_fmul      0x92
#define psl_fdiv      0x93
#define psl_fmod      0x94
#define psl_fadd      0x96
#define psl_fsub      0x97
#define psl_flt       0x98
#define psl_flte      0x99
#define psl_feq       0x9A
#define psl_fne       0x9B
#define psl_fgt       0x9C
#define psl_fgte      0x9D
#define psl_version   0xC0
#define psl_dsrcfile  0xD0
#define psl_dsrcline  0xD1
#define psl_dsrccol   0xD2
#define psl_print     0xE0
#define psl_debug     0xE1
#define psl_intrinsic 0xE2
#define psl_trap      0xED
#define psl_include   0xEE
#define psl_parse     0xEF
#define psl_gadd      0xF0
#define psl_grem      0xF1
#define psl_gcommit   0xFB
#define psl_marker    0xFC
#define psl_immediate 0xFD
#define psl_code      0xFE
#define psl_raw       0xFF

#define psl_dlopen    0xC1
#define psl_dlclose   0xC2
#define psl_dlsym     0xC3
#define psl_cget      0xC4
#define psl_cset      0xC5
#define psl_cinteger  0xC6
#define psl_ctype     0xC7
#define psl_cstruct   0xC8
#define psl_csizeof   0xC9
#define psl_csget     0xCA
#define psl_csset     0xCB
#define psl_prepcif   0xCC
#define psl_ccall     0xCD

/* ctypes */
#define psl_ctype_void          0
#define psl_ctype_int           1
#define psl_ctype_float         2
#define psl_ctype_double        3
#define psl_ctype_long_double   4
#define psl_ctype_uint8         5
#define psl_ctype_int8          6
#define psl_ctype_uint16        7
#define psl_ctype_int16         8
#define psl_ctype_uint32        9
#define psl_ctype_int32         10
#define psl_ctype_uint64        11
#define psl_ctype_int64         12
#define psl_ctype_pointer       14
#define psl_ctype_uchar         24
#define psl_ctype_schar         25
#define psl_ctype_ushort        26
#define psl_ctype_sshort        27
#define psl_ctype_uint          28
#define psl_ctype_sint          29
#define psl_ctype_ulong         30
#define psl_ctype_slong         31
#define psl_ctype_ulonglong     32
#define psl_ctype_slonglong     33

/* special names */
#define PSL_SELF_PROCEDURE      "+procedure"
#define PSL_EXCEPTION_STACK     "+exception"

#endif
