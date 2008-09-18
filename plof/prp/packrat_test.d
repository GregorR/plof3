/**
 * Test for plof.prp.packrat
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

module plof.prp.packrat_test;

import tango.io.Stdout;

import tango.math.random.Kiss;

import tango.time.StopWatch;

import plof.prp.packrat;

int main()
{
    auto addexp = new Production("addexp");
    auto plus = new RegexTerminal("plus", "\\+");
    auto mulexp = new Production("mulexp");
    auto star = new RegexTerminal("star", "\\*");
    auto num = new RegexTerminal("num", "[0-9]+");

    addexp.addChoice([cast(GrammarElem) addexp, plus, mulexp]);
    addexp.addChoice([cast(GrammarElem) mulexp]);
    mulexp.addChoice([cast(GrammarElem) mulexp, star, num]);
    mulexp.addChoice([cast(GrammarElem) num]);

    // test with strings of length 1-999
    char[] inp;
    StopWatch sw;
    for (int l = 1; l <= 999; l += 2) {
        inp.length = l;

        // put in random numbers
        for (int j = 0; j < l; j += 2) {
            inp[j] = (Kiss.shared.toInt % 10) + '0';
        }
        // and random operators
        for (int j = 1; j < l; j += 2) {
            if (Kiss.shared.toInt & 0x1) {
                inp[j] = '*';
            } else {
                inp[j] = '+';
            }
        }

        // then time it
        sw.start();
        addexp.clear();
        addexp.parse(inp, 0, "", 0, 0).gc.blessDown();
        real time = sw.stop();
        Stdout(l)(": ").format("{0:10}", time).newline;
    }

    return 0;
}

/*void dumpParseResult(ParseResult* pr, char[] spaces = "")
{
    Stdout(spaces)(pr.prod.name)("(")(pr.choice)(") \"")(pr.consumed)("\"").newline;
    foreach (sr; pr.subResults.arr) {
        dumpParseResult(sr, spaces~"  ");
    }
}*/
