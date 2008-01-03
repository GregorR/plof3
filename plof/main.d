/**
 * Main frontend for Plof
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

module plof.main;

import tango.io.Console;
import tango.io.File;
import tango.io.Stdout;

import plof.prp.prp;

import plof.psl.file;
import plof.psl.interp;
import plof.psl.pslobject;

int main(char[][] args)
{
    char[][] pslFiles;
    char[][] plofFiles;
    char[] outFile;
    ubyte[] outPSL;
    bool interactive;

    /// Parse arguments for PSL or Plof files
    int argi;
    for (argi = 1; argi < args.length; argi++) {
        if (args[argi][0] != '-') {
            // The Plof file actually being run, so add it
            plofFiles ~= args[argi];
            argi++;
            break;
        }

        if (args.length > argi + 1) {
            if (args[argi] == "-s") {
                // PSL file, include it
                pslFiles ~= args[++argi];

            } else if (args[argi] == "-i") {
                // Plof file
                plofFiles ~= args[++argi];

            } else if (args[argi] == "-c") {
                // Compile
                outFile = args[++argi];

            } else {
                Stderr("Unrecognized argument ")(args[argi]).newline;
                return 1;

            }

        } else {
            if (args[argi] == "--help" ||
                args[argi] == "-h") {
                usage();
                return 0;

            } else if (args[argi] == "--interactive" ||
                       args[argi] == "-I") {
                interactive = true;

            } else {
                Stderr("Unrecognized argument ")(args[argi]).newline;
                return 1;

            }

        }
    }

    // If no files were specified at all, WTF?
    if (pslFiles.length == 0 &&
        plofFiles.length == 0 &&
        interactive == false) {
        usage();
        return 1;
    }

    // If no base PSL file was supplied, use std.psl
    if (pslFiles.length == 0) {
        pslFiles ~= "std.psl";
    }

    // Now set up a base environment ...
    PSLStack* stack = PSLStack.allocate();
    stack.gc.blessUp();
    PSLObject* context = PSLObject.allocate(pslNull);
    context.gc.blessUp();
    PlofRuntimeParser prp = new PlofRuntimeParser;

    // Interpret all the PSL files ...
    foreach (pslFile; pslFiles) {
        ubyte[] psl = cast(ubyte[]) (new File(pslFile)).read();
        psl = pslProgramData(psl);

        // Run the immediates
        interpret(psl, stack, context, true, prp);
        stack.truncate(0);

        // Then run or compile the rest
        if (outFile.length) {
            outPSL ~= psl;
        } else {
            interpret(psl, stack, context);
            stack.truncate(0);
        }
    }

    // Interpret all of the Plof files ...
    foreach (plofFile; plofFiles) {
        char[] fcont = cast(char[]) (new File(plofFile)).read();
        ubyte[] psl = prp.parse(fcont, cast(ubyte[]) "top");
        if (fcont.length) {
            Stderr("Failed to parse! Remaining: '")(fcont)("'").newline;
        }
        
        // Run or compile it
        if (outFile.length) {
            outPSL ~= psl;
        } else {
            interpret(psl, stack, context);
            stack.truncate(0);
        }
    }

    // Then interactive mode
    if (interactive) {
        char[] code, line;
        ubyte[] psl;

        while (true) {
            // read
            if (code.length) {
                Stdout("    # ").flush();
            } else {
                Stdout("Plof> ").flush();
            }
            line = Cin.copyln();
            if (line == "") {
                // blank line, cancel whatever we're doing
                code = "";
                continue;
            } else {
                code ~= line ~ "\n";
            }

            // parse
            psl = prp.parse(code, cast(ubyte[]) "top");

            // Run or compile it
            if (outFile.length) {
                outPSL ~= psl;
            } else {
                interpret(psl, stack, context);
                stack.truncate(0);
            }
        }
    }

    // Now write out the PSL file
    if (outFile.length) {
        // Make the PSL file
        (new File(outFile)).write(makePSLFile(outPSL));
    }

    return 0;
}

/// Usage statement
void usage()
{
    Stderr
    ("Use: dplof [-s<PSL file>] [-i<plof file>] [-c<output file>] <plof file>").newline()
    ("Options:").newline()
    ("  -s <PSL file>:          Include the provided PSL file instead of").newline()
    ("                          std.psl").newline()
    ("  -i <plof file>:         Include the provided Plof file before the one").newline()
    ("                          being interpreted.").newline()
    ("  -c <output file>:       Compile into PSL only, do not run.").newline()
    ("  -I:                     Interactive mode.").newline();
}
