/**
 * The Plof Runtime Parser
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

module plof.prp.prp;

import tango.io.Stdout;

import tango.stdc.stdlib;

import plof.prp.iprp;
import plof.prp.packrat;

import plof.psl.bignum;
import plof.psl.interp;
import plof.psl.pslobject;

/// Enable debug output
bool enableDebug = false;

/// The Plof runtime parser
class PlofRuntimeParser : IPlofRuntimeParser {
    /// Parse the given code into PSL code, given a top
    ubyte[] parse(ref char[] code, ubyte[] top,
                  char[] file, uint line = 0, uint col = 0)
    {
        ubyte[] psl;

        // Parse the code chunk-by-chunk
        while (code.length != 0) {
            GrammarElem topg = _grammar[top];
            ParseResultArr res = topg.parse(code, 0, file, line, col);
            topg.clear();

            if (res.arr.length == 0) {
                // Failed to parse, bail out
                return psl;

            } else {
                // Run the associated commands to generate the PSL
                PSLObject ret = cast(PSLObject) res.arr[0].runCommands();

                // should have returned raw data
                if (!ret.isArray && ret.raw !is null) {
                    // add it to the output
                    psl ~= ret.raw.data;

                    // and then run immediates
                    interpret(ret.raw.data, true, this);
                }

                // bump the line and col
                line = res.arr[0].eline;
                col = res.arr[0].ecol;

                // Then take out whatever was parsed from the code
                code = res.arr[0].remaining;
            }
        }

        return psl;
    }

    /// Add a grammar element
    void gadd(ubyte[] name, ubyte[][] target, ubyte[] psl)
    {
        // make sure the production exists
        UProduction* production = name in _ugrammar;
        if (production is null) {
            _ugrammar[name] = null;
            production = name in _ugrammar;
        }

        // then add the target
        (*production) ~= (target ~ psl);
    }

    /// Remove a grammar element
    void grem(ubyte[] name)
    {
        _ugrammar.remove(name);
    }

    /// Commit the grammar
    void gcommit()
    {
        // Get rid of the current grammar
        _grammar = null;

        //Stdout.newline().newline()("COMMIT").newline;

        // Then go production by production ...
        foreach (productionName, production; _ugrammar) {
            Production tproduction;
            if (productionName in _grammar)
                tproduction = _grammar[productionName];
            else
                tproduction = _grammar[productionName] =
                    new Production(cast(char[]) productionName);
            ParserPSL psltarget = new ParserPSL(this);
            psltarget.code.length = production.length;

            // Give it the appropriate callback
            tproduction.command = &psltarget.interp;

            // Target by target ...
            foreach (tnum, target; production) {
                //Stdout(cast(char[]) productionName)(" =");

                GrammarElem[] ttarget;
                ttarget.length = target.length; // preallocate
                ttarget.length = 0;

                // Element by element ...
                foreach (elem; target[0..$-1]) {
                    //Stdout(" ")(cast(char[]) elem);

                    // Regex or not
                    if (elem[0] == '/') {
                        // Regex, so make a regex element
                        ttarget ~= new RegexTerminal(cast(char[]) elem,
                                                     cast(char[]) elem[1..$-1]);

                        // And add the appropriate callback
                        ttarget[$-1].command = &regexCallback;

                    } else {
                        // Not a regex, get the proper production
                        Production subprod;
                        if (elem in _grammar)
                            subprod = _grammar[elem];
                        else
                            subprod = _grammar[elem] =
                                new Production(cast(char[]) elem);

                        // Refer to it
                        ttarget ~= subprod;
                    }
                }

                //Stdout.newline;

                // Add the choice
                tproduction.addChoice(ttarget);

                // The code is in the last element
                psltarget.code[tnum] = target[$-1];
            }
        }
    }

    /// Callback for regexes
    void* regexCallback(ParseResult res, void*[] args)
    {
        // make a command to generate the result
        ubyte[] bignum;
        bignum.length = bignumBytesReq(res.consumed.length);
        intToPSLBignum(res.consumed.length, bignum);
        ubyte[] cmd = ([cast(ubyte) 0xFF] ~
                       bignum ~
                       cast(ubyte[]) res.consumed);

        // put it in a PSLRawData
        PSLRawData rd = new PSLRawData(cmd);

        // and in a PSLObject
        PSLObject no = new PSLObject(pslNull);
        no.raw = rd;

        return cast(void*) no;
    }

    private Production[ubyte[]] _grammar;
    private UProduction[ubyte[]] _ugrammar;
}

/// An uncommitted grammar element (within RHS of a production)
alias ubyte[] UGrammarElem;

/// An uncommitted RHS of a production
alias UGrammarElem[] UGRHS;

/// An uncommitted production
alias UGRHS[] UProduction;

/// A class wrapping PSL code from the parser
class ParserPSL {
    this(PlofRuntimeParser sprp) { prp = sprp; }

    /// Interpret the associated code with the given PSLObject args
    void* interp(ParseResult res, void*[] args)
    {
        // Create a stack,
        PSLStack stack = new PSLStack();

        // and a master context,
        PSLObject context = new PSLObject(pslNull);

        // an array to store the arguments,
        PSLArray arr = new PSLArray(cast(PSLObject[]) args);

        // and put it in an object,
        PSLObject no = new PSLObject(context);
        no.arr = arr;

        // put the object on the stack
        stack.stack ~= no;

        // and run it
        interpret(code[res.choice], stack, context, false, prp);

        // get the top element off the stack
        if (stack.stack.length < 1) {
            Stderr("Parser error: Production ")(res.prod.name)(" returned nothing.").newline;
            no = pslNull;
        } else {
            no = stack.stack[$-1];
        }

        if (enableDebug &&
            !no.isArray && no.raw !is null &&
            no.raw.data.length > ptrdiff_t.sizeof) {
            // add debugger info
            int debugbnr = bignumBytesReq(res.file.length);
            int debuglen = res.file.length + debugbnr + 18;
                /* 18 = 8 commands + two 1-byte bignums + two 4-byte integers =
                 * 8 + 1 + 1 + 4 + 4 = 18 */
            ubyte[] debugbuf = (cast(ubyte*) alloca(debuglen + no.raw.data.length))
                [0..(debuglen + no.raw.data.length)];

            // first the filename
            int i = 0;
            debugbuf[i] = '\xFF'; // raw
            i++;
            intToPSLBignum(res.file.length, debugbuf[i..i+debugbnr]);
            i += debugbnr;
            debugbuf[i .. i + res.file.length] =
                cast(ubyte[]) res.file;
            i += res.file.length;
            debugbuf[i] = '\xD0'; // dsrcfile
            i++;

            // then the line number
            debugbuf[i..i+2] = cast(ubyte[]) "\xFF\x04"; // raw, length 4
            i += 2;
            debugbuf[i] = cast(ubyte) ((res.sline & 0xFF000000) >> 24);
            debugbuf[i+1] = cast(ubyte) ((res.sline & 0xFF0000) >> 16);
            debugbuf[i+2] = cast(ubyte) ((res.sline & 0xFF00) >> 8);
            debugbuf[i+3] = cast(ubyte) (res.sline & 0xFF);
            i += 4;
            debugbuf[i..i+2] = cast(ubyte[]) "\x70\xD1"; // integer dsrcline
            i += 2;

            // then the column
            debugbuf[i..i+2] = cast(ubyte[]) "\xFF\x04"; // raw, length 4
            i += 2;
            debugbuf[i] = cast(ubyte) ((res.scol & 0xFF000000) >> 24);
            debugbuf[i+1] = cast(ubyte) ((res.scol & 0xFF0000) >> 16);
            debugbuf[i+2] = cast(ubyte) ((res.scol & 0xFF00) >> 8);
            debugbuf[i+3] = cast(ubyte) (res.scol & 0xFF);
            i += 4;
            debugbuf[i..i+2] = cast(ubyte[]) "\x70\xD2"; // integer dsrccol
            i += 2;

            // finally, the original function
            debugbuf[i..$] = no.raw.data[];

            no.raw = new PSLRawData(debugbuf);
        }

        return cast(void*) no;
    }

    private ubyte[][] code;

    // the runtime parser that created this
    PlofRuntimeParser prp;
}
