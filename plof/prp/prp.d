/**
 * The Plof Runtime Parser
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

module plof.prp.prp;

import plof.prp.iprp;
import plof.prp.packrat;

import plof.psl.bignum;
import plof.psl.interp;
import plof.psl.pslobject;

/// The Plof runtime parser
class PlofRuntimeParser : IPlofRuntimeParser {
    /// Parse the given code into PSL code, given a top
    ubyte[] parse(ref char[] code, ubyte[] top)
    {
        ubyte[] psl;

        // Parse the code chunk-by-chunk
        while (code.length != 0) {
            GrammarElem topg = _grammar[top];
            ParseResultArr* res = topg.parse(code, 0);
            topg.clear();

            if (res.arr.length == 0) {
                // Failed to parse, bail out
                res.gc.blessDown();
                return psl;

            } else {
                // Run the associated commands to generate the PSL
                PSLObject* ret = cast(PSLObject*) res.arr[0].runCommands();

                // should have returned raw data
                if (!ret.isArray && ret.raw !is null) {
                    // add it to the output
                    psl ~= ret.raw.data;

                    // and then run immediates
                    interpret(ret.raw.data, true, this);
                }

                // get rid of the data it returned
                ret.gc.blessDown();

                // Then take out whatever was parsed from the code
                code = res.arr[0].remaining;
            }

            res.gc.blessDown();
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

        // Then go production by production ...
        foreach (productionName, production; _ugrammar) {
            Production tproduction;
            if (productionName in _grammar)
                tproduction = _grammar[productionName];
            else
                tproduction = _grammar[productionName] =
                    new Production(cast(char[]) productionName);
            ParserPSL psltarget = new ParserPSL;
            psltarget.code.length = production.length;

            // Give it the appropriate callback
            tproduction.command = &psltarget.interp;

            // Target by target ...
            foreach (tnum, target; production) {
                GrammarElem[] ttarget;
                ttarget.length = target.length; // preallocate
                ttarget.length = 0;

                // Element by element ...
                foreach (elem; target[0..$-1]) {
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

                // Add the choice
                tproduction.addChoice(ttarget);

                // The code is in the last element
                psltarget.code[tnum] = target[$-1];
            }
        }
    }

    /// Callback for regexes
    void* regexCallback(ParseResult* res, void*[] args)
    {
        // make a command to generate the result
        ubyte[] bignum;
        bignum.length = bignumBytesReq(res.consumed.length);
        intToPSLBignum(res.consumed.length, bignum);
        ubyte[] cmd = ([cast(ubyte) 0xFF] ~
                       bignum ~
                       cast(ubyte[]) res.consumed);

        // put it in a PSLRawData
        PSLRawData* rd = PSLRawData.allocate(cmd);

        // and in a PSLObject
        PSLObject* no = PSLObject.allocate(pslNull);
        no.raw = rd;
        no.gc.blessUp();

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
    /// Interpret the associated code with the given PSLObject* args
    void* interp(ParseResult* res, void*[] args)
    {
        // Create a stack,
        PSLStack* stack = PSLStack.allocate();
        stack.gc.blessUp();

        // and a master context,
        PSLObject* context = PSLObject.allocate(pslNull);
        context.gc.blessUp();

        // an array to store the arguments,
        PSLArray* arr = PSLArray.allocate(cast(PSLObject*[]) args);

        // and put it in an object,
        PSLObject* no = PSLObject.allocate(context);
        no.arr = arr;

        // put the object on the stack
        stack.stack ~= no;
        no.gc.refUp();

        // and run it
        interpret(code[res.choice], stack, context);

        // get the top element off the stack
        no = stack.stack[$-1];

        // reference it, deref the array
        no.gc.blessUp();
        foreach (i; args) {
            (cast(PSLObject*) i).gc.blessDown();
        }

        // then get rid of them
        context.gc.blessDown();
        stack.gc.blessDown();

        return no;
    }

    private ubyte[][] code;
}
