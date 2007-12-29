/**
 * Convenience functions for PSL's bignums
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

module plof.psl.bignum;

/// Convert a PSL bignum to a uint. Returns the number of bytes consumed
uint pslBignumToInt(ubyte[] bignum, out uint val)
{
    uint ret = 0;
    uint c = 0;

    // just go byte-by-byte, increasing
    foreach (v; bignum) {
        c++;
        ret <<= 7;
        ret += (v & 0x7F);
        if (v < 128) break;
    }

    val = ret;
    return c;
}

/// Calculate the number of bytes required to store a bignum
uint bignumBytesReq(uint val)
{
    uint c = 0;
    do {
        val >>= 7;
        c++;
    } while (val > 0);

    return c;
}

/// Convert a uint to a PSL bignum, in the provided buffer
void intToPSLBignum(uint val, ubyte[] buf)
{
    for (int i = buf.length - 1; i >= 0; i--) {
        buf[i] = (val % 128);
        if (i != buf.length - 1) {
            buf[i] ^= 0x80;
        }
        val >>= 7;
    }
}
