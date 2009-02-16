/**
 * Frontend for PSL interpreter
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

module plof.psl.psli;

import tango.io.Console;
import tango.io.File;

import tango.math.Math;

import plof.psl.file;
import plof.psl.interp;

int main(char[][] args)
{
    ubyte[] psl;

    // if we were provided a file, use that
    if (args.length > 1) {
        psl = cast(ubyte[]) (new File(args[1])).read();

    } else {
        ubyte[1024] buf;
        uint rd;

        // read chunk-by-chunk until there's nothing left to read
        while ((rd = Cin.stream.read(cast(ubyte[]) buf)) != 0) {
            psl ~= buf[0..rd];
        }
    }

    // now pull out the program data
    psl = pslProgramData(psl);

    // and run it
    interpret(psl);

    return 0;
}
