/*
 * PSL disassembler
 *
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

#include <stdio.h>
#include <string.h>

#include <gc/gc.h>

#include "bignum.h"
#include "parse.h"
#include "plof.h"
#include "psl.h"
#include "pslfile.h"

#define BUFSTEP 1024

void psldasm(int indent, size_t sz, unsigned char *psl, FILE *out);

int main(int argc, char **argv)
{
    FILE *pslf;
    struct Buffer_psl psl;

    GC_INIT();

    if (argc != 2) {
        fprintf(stderr, "Use: psldasm <file>\n");
        return 1;
    }

    /* open the file */
    pslf = fopen(argv[1], "rb");
    if (pslf == NULL) {
        perror(argv[1]);
        return 1;
    }

    /* read in the buffer */
    INIT_BUFFER(psl);
    READ_FILE_BUFFER(psl, pslf);
    fclose(pslf);

    /* get the desired data */
    psl = readPSLFile(psl.bufused, psl.buf);

    /* now disassemble */
    psldasm(0, psl.bufused, psl.buf, stdout);

    return 0;
}

void escapedString(size_t sz, unsigned char *str, FILE *out)
{
    size_t i;

    for (i = 0; i < sz; i++) {
        switch (str[i]) {
            case '"':
                fprintf(out, "\\\"");
                break;

            default:
                fprintf(out, "%c", str[i]);
        }
    }
}

void outIndent(int level, FILE *out)
{
    int i;
    for (i = 0; i < level; i++) {
        fputc(' ', out);
    }
}

void psldasm(int indent, size_t sz, unsigned char *psl, FILE *out)
{
    int psli;

    for (psli = 0; psli < sz; psli++) {
        unsigned char cmd = psl[psli], ncmd;
        size_t rawsz = 0;
        unsigned char *raw;
        outIndent(indent, out);

        /* check if it has raw data */
        if (cmd >= psl_marker) {
            psli++;
            psli += pslBignumToInt(psl + psli, &rawsz);
            raw = psl + psli;
            psli += rawsz - 1;

            ncmd = 0;
            if (psli < sz - 1) {
                ncmd = psl[psli+1];
            }

            /* if this is 'raw' followed by an integer, just write the number */
            if (cmd == psl_raw && rawsz == 4 && ncmd == psl_integer) {
                fprintf(out, "%d\n",
                        (raw[0] << 24) |
                        (raw[1] << 16) |
                        (raw[2] << 8) |
                        (raw[3]));
                psli++;

            } else {
                /* output the raw */
                switch (cmd) {
                    case psl_immediate:
                        fprintf(out, "[\n");
                        psldasm(indent + 2, rawsz, raw, out);
                        outIndent(indent, out);
                        fprintf(out, "]\n");
                        break;
    
                    case psl_code:
                        fprintf(out, "{\n");
                        psldasm(indent + 2, rawsz, raw, out);
                        outIndent(indent, out);
                        fprintf(out, "}\n");
                        break;
    
                    default:
                        fprintf(out, "\"");
                        escapedString(rawsz, raw, out);
                        fprintf(out, "\"\n");
                }
            }

        } else {
            /* now write something */
            switch (cmd) {
#include "psldasm-cmds.c"

                default:
                    fprintf(out, "UNRECOGNIZED\n");
            }

        }
    }
}
