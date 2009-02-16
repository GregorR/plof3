/**
 * Frontend assembler for ASCII PSL
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

module plof.psl.pslasm;

import tango.io.File;
import tango.io.FilePath;
import tango.io.Stdout;

import plof.psl.compile;

int main(char[][] args)
{
    if (args.length < 2) {
        Stderr("Use: pslasm <input file> [output file]").newline;
        return 1;
    }

    // figure out the input and output files
    char[] inpfn = args[1];
    auto inpfp = new FilePath(inpfn);
    char[] outpfn;
    if (args.length >= 3) {
        outpfn = args[2];
    } else {
        auto outpfp = inpfp.dup.suffix("psl");
        if (inpfp == outpfp) {
            Stderr("Can't determine an output file name.").newline;
        }
        outpfn = outpfp.toString();
    }

    // now parse and compile ...
    char[] cont = cast(char[]) (new File(args[1])).read();
    ubyte[] outcont = pslCompile(cont);

    // wrap it in a .psl file
    outcont = [cast(ubyte) 0x9E, 0x50, 0x53, 0x4C, 0x17, 0xF2, 0x58, 0x8C] ~
              pslBignum(outcont.length + 1) ~
              [cast(ubyte) 0x00] ~
              outcont;

    // then output
    if (outpfn == "-") {
        Stdout(cast(char[]) outcont).flush();
    } else {
        (new File(outpfn)).write(outcont);
    }
    return 0;
}
