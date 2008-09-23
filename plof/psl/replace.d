/**
 * Implementation for the 'replace' operation
 *
 * License:
 *  Copyright (c) 2008  Gregor Richards
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

module plof.psl.replace;

import plof.psl.bignum;
import plof.psl.psl;

/// PSL replace
ubyte[] pslfreplace(ubyte[] rin, ubyte[] marker, ubyte[] to)
{
    // reserve output space
    ubyte[] rout;
    rout.length = rin.length;
    rout.length = 0;

    // now go through rin
    for (int i = 0; i < rin.length; i++) {
        // get the command
        ubyte cmd = rin[i];
        bool hassub = false;
        ubyte[] sub = null;

        // does it have raw data?
        if (cmd >= psl_marker) {
            hassub = true;
            i++;
            uint len;
            i += pslBignumToInt(rin[i..$], len);
            sub = rin[i..i+len];
            i += len - 1;
        }

        if (cmd == psl_marker &&
            sub == marker) {
            // if it's the marker, replace it
            rout ~= to;

        } else if (cmd == psl_immediate ||
                   cmd == psl_code) {
            // replace it in code
            sub = pslfreplace(sub, marker, to);

        }

        // now put it in the output
        rout ~= cmd;

        if (hassub) {
            // make the bignum
            ubyte[] bn;
            bn.length = bignumBytesReq(sub.length);
            intToPSLBignum(sub.length, bn);

            // and put it together
            rout ~= bn ~ sub;
        }
    }

    return rout;
}
