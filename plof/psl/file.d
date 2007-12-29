/**
 * Convenience function for .psl files
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

module plof.psl.file;

import plof.psl.bignum;

/// Get the PSL program data out of a PSL file
ubyte[] pslProgramData(ubyte[] f)
{
    // skip over the header
    f = f[8..$];

    // now go section by section
    for (uint i = 0; i < f.length;) {
        uint slen, stype, stl;

        // get the section length
        i += pslBignumToInt(f[i..$], slen);

        // the section type
        stl = pslBignumToInt(f[i..$], stype);

        // if it's program data, we're done
        if (stype == 0) {
            return f[i+stl..i+slen];

        } else {
            // skip this section
            i += slen;
        }
    }

    // didn't find program data! Whoops!
    return [];
}

/// Put the PSL program data into a PSL file
ubyte[] makePSLFile(ubyte[] psl)
{
    ubyte[] bignum;
    bignum.length = bignumBytesReq(psl.length + 1);
    intToPSLBignum(psl.length + 1, bignum);
    ubyte[] outp = ([cast(ubyte) 0x9E, 0x50, 0x53, 0x4C, 0x17, 0xF2, 0x58, 0x8C] ~
                    bignum ~
                    [cast(ubyte) 0x00] ~
                    psl);
    return outp;
}
