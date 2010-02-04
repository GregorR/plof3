/*
 * PSL file exporter/importer
 *
 * Copyright (C) 2009, 2010 Gregor Richards
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

#ifndef PLOF_PSLFILE_H
#define PLOF_PSLFILE_H

#include <stdio.h>

#include "plof.h"

#define PSL_FILE_MAGIC "\x9E\x50\x53\x4C\x17\xF2\x58\x8C"
#define PSL_SECTION_PROGRAM_DATA                0
#define PSL_SECTION_COMMENT                     1
#define PSL_SECTION_STRIPPED_PROGRAM_DATA       2
#define PSL_SECTION_RAW_DATA_TABLE              3

/* return true if this buffer points to a PSL file */
int isPSLFile(size_t sz, unsigned char *buf);

/* read in the PSL from a PSL file */
struct Buffer_psl readPSLFile(size_t sz, unsigned char *buf);

/* write out PSL to a file */
void writePSLFile(FILE *to, size_t sz, unsigned char *buf);

/* unstrip PSL */
struct Buffer_psl unstripPSL(struct Buffer_psl psls, struct Buffer_psl strtab);

/* strip PSL */
void stripPSL(struct Buffer_psl psl, struct Buffer_psl *psls, struct Buffer_psl *strtab);

#endif
