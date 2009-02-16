/**
 * Lexer for ASCII PSL
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

module plof.psl.lex;

/** Lex PSL assembly */
char[][] pslLex(char[] inp)
{
    char[][] ret;
    int start = 0;
    int i;

    for (i = 0; i < inp.length; i++) {
        if (inp[i] == ' ' || inp[i] == '\t' ||
            inp[i] == '\r' || inp[i] == '\n') {
            // whitespace, push this as a token
            if (start != i)
                ret ~= inp[start..i];
            start = i + 1;

        } else if (inp[i] == '{' || inp[i] == '[') {
            // start a procedure
            if (start != i)
                ret ~= inp[start..i];
            start = i;

            int depth = 1;
            for (i++; i < inp.length && depth > 0; i++) {
                if (inp[i] == '{' || inp[i] == '[') {
                    depth++;
                } else if (inp[i] == '}' || inp[i] == ']') {
                    depth--;
                }
            }

            // push on the procedure
            ret ~= inp[start..i];
            start = i + 1;

        } else if (inp[i] == '"') {
            // start a string
            if (start != i)
                ret ~= inp[start..i];
            start = i;

            // find the end of the string
            for (i++; i < inp.length && inp[i] != '"'; i++) {}

            // push it
            ret ~= inp[start..i+1];
            start = i + 1;

        }
    }

    if (start != i)
        ret ~= inp[start..i];

    return ret;
}
