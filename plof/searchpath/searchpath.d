/**
 * Find files on Plof's search path
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

module plof.searchpath.searchpath;

import tango.io.FilePath;
import tango.io.Stdout;

import tango.stdc.stdlib;

import plof.prp.prp;

char[] plofExeFP;

/// Find the given files on the search path
void plofSearchPath(char[][] files) {
    for (int i = 0; i < files.length; i++) {
        auto fp = new FilePath(files[i]);
        if (!fp.isAbsolute()) {
            // not absolute, so find it
            if (!fp.exists()) {
                // in some search path
                FilePath fp2;
                if (plof.prp.prp.enableDebug) {
                    // 1) share/plof/include/debug
                    fp2 = new FilePath(FilePath.join(
                                [plofExeFP, "../share/plof/include/debug", fp.toString()]));
                    if (fp2.exists()) {
                        files[i] = fp2.toString();
                        continue;
                    }
                    // 2) plof_include/debug
                    fp2 = new FilePath(FilePath.join(
                        [plofExeFP, "plof_include/debug", fp.toString()]));
                    if (fp2.exists()) {
                        files[i] = fp2.toString();
                        continue;
                    }
                }

                // 3) share/plof/include
                fp2 = new FilePath(FilePath.join(
                    [plofExeFP, "../share/plof/include", fp.toString()]));
                if (fp2.exists()) {
                    files[i] = fp2.toString();
                    continue;
                }

                // 4) plof_include
                fp2 = new FilePath(FilePath.join(
                    [plofExeFP, "plof_include", fp.toString()]));
                if (fp2.exists()) {
                    files[i] = fp2.toString();
                    continue;
                }

                Stderr("File ")(files[i])(" not found!").newline;
                exit(1);
            }
        }
    }
}
