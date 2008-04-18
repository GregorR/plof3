/**
 * A simple Packrat parser in D, for the Plof Runtime Parser
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

module plof.prp.packrat;

//import tango.io.Stdout;

import tango.stdc.stdlib;
import tango.stdc.stringz;

import plof.psl.gc;

import bcd.pcre.pcre;
version (build) { pragma(link, "pcre"); }

/** A grammatical element: Could be a terminal or nonterminal */
class GrammarElem {
    /** Parse this grammar element, memoized */
    ParseResultArr* parse(char[] inp, uint start, char[] file, uint line, uint col)
    {
        // if we don't have room to memoize this, make room
        if (memo.length <= start) {
            int oldlen = memo.length;
            memo = (cast(ParseResultArr**) realloc(memo.ptr, (start + 1) * (ParseResultArr*).sizeof))
                [0..(start+1)];
            memo[oldlen..$] = null;
        }

        // check for already-memoized results
        ParseResultArr* pr = memo[start];
        if (pr !is null) {
            pr.gc.blessUp();
            return pr;
        }

        // otherwise, check and memoize
        pr = gparse(inp, start, file, line, col);
        pr.gc.blessUp();
        memo[start] = pr;
        return pr;
    }

    /** Clear memoization cache */
    void clear()
    {
        if (memo.length == 0) return;

        // deref ...
        foreach (me; memo) {
            if (me !is null)
                me.gc.blessDown();
        }

        // and free
        free(memo.ptr);
        memo = null;

        subclear();
    }

    /** Clear subelements (generally a noop) */
    void subclear() {}

    /** Parse this grammar element over the given input (non-memoized) */
    ParseResultArr* gparse(char[] inp, uint start, char[] file, uint line, uint col)
    { return null; }

    /** The name of this element */
    char[] name() { return _name; }

    /** The command associated with this production (if applicable) */
    void* delegate(ParseResult*, void*[]) command;

    private:

    char[] _name;

    /** Memoization store */
    ParseResultArr*[] memo;
}

/** The result of parsing */
struct ParseResult {
    PlofGCable gc;

    static ParseResult* allocate(ParseResult* copy)
    {
        ParseResult* ret = PlofGCable.allocate!(ParseResult)();
        ret.gc.foreachRef = &ret.foreachRef;
        ret.prod = copy.prod;
        ret.choice = copy.choice;
        ret.subResults = ParseResultArr.allocate(copy.subResults);
        ret.subResults.gc.refUp();
        ret.file = copy.file;
        ret.sline = copy.sline;
        ret.eline = copy.eline;
        ret.scol = copy.scol;
        ret.ecol = copy.ecol;
        return ret;
    }

    static ParseResult* allocate(GrammarElem sprod)
    {
        ParseResult* ret = PlofGCable.allocate!(ParseResult)();
        ret.gc.foreachRef = &ret.foreachRef;
        ret.prod = sprod;
        ret.choice = 0;
        ret.subResults = ParseResultArr.allocate();
        ret.subResults.gc.refUp();
        return ret;
    }

    /** Foreach ref over sub results */
    void foreachRef(void delegate(PlofGCable*) callback)
    {
        callback(cast(PlofGCable*) subResults);
    }

    /** Run the commands associated with this result */
    void* runCommands()
    {
        // First run the commands of the sub-results
        void*[] args = (cast(void**) alloca(subResults.arr.length * (void*).sizeof))
            [0..(subResults.arr.length)];
        foreach (i, sr; subResults.arr) {
            args[i] = sr.runCommands();
        }

        // Then the command associated with this production
        if (prod.command is null) {
            return null;
        } else {
            return prod.command(this, args);
        }
    }

    /** The element which produced this result */
    GrammarElem prod;

    /** The choice used (if multiple) */
    int choice;

    /** Any sub-results */
    ParseResultArr* subResults;

    /** The file being parsed */
    char[] file;

    /** The line this result begins and ends on */
    uint sline, eline;

    /** The column this result begins and ends on */
    uint scol, ecol;

    /** The string consumed by this parse */
    char[] consumed;
    
    /** The string remaining after this parse */
    char[] remaining;
}

/** An array of parse results */
struct ParseResultArr {
    PlofGCable gc;

    /// Allocate empty
    static ParseResultArr* allocate()
    {
        ParseResultArr* ret = PlofGCable.allocate!(ParseResultArr)();
        ret.gc.foreachRef = &ret.foreachRef;
        ret.gc.destructor = &ret.destructor;
        return ret;
    }

    /// Allocate with a provided array
    static ParseResultArr* allocate(ParseResultArr* copy)
    {
        ParseResultArr* ret = PlofGCable.allocate!(ParseResultArr)();
        ret.gc.foreachRef = &ret.foreachRef;
        ret.gc.destructor = &ret.destructor;
        ret.arr = (cast(ParseResult**) malloc(copy.arr.length * (ParseResult*).sizeof))
            [0..copy.arr.length];
        ret.arr[] = copy.arr[];

        // then ref them up
        foreach (e; ret.arr) {
            e.gc.refUp();
        }

        return ret;
    }

    void foreachRef(void delegate(PlofGCable*) callback)
    {
        foreach (e; arr) {
            callback(cast(PlofGCable*) e);
        }
    }

    void destructor()
    {
        free(arr.ptr);
    }

    /// Append to this array
    void append(ParseResult* e)
    {
        // extend the array
        arr = (cast(ParseResult**) realloc(arr.ptr, (arr.length + 1) * (ParseResult*).sizeof))
            [0..(arr.length + 1)];

        // then add the element
        arr[$-1] = e;

        // and ref it
        e.gc.refUp();
    }

    /// The array itself
    ParseResult*[] arr;
}

/** A production (a nonterminal in usual nomenclature) */
class Production : GrammarElem {
    this(char[] sname) {
        _name = sname.dup;
        sub = null;
    }

    /** Add a choice to this production */
    void addChoice(GrammarElem[] ch) {
        sub ~= ch;
    }

    /** Clear subelements */
    void subclear()
    {
        foreach (sge; sub) {
            foreach (ge; sge) {
                ge.clear();
            }
        }
        foreach (ge; lRecursions) {
            if (ge !is null) ge.clear();
        }
    }

    /** Parse this production, non-memoized */
    ParseResultArr* gparse(char[] inp, uint start, char[] file, uint line, uint col)
    {
        //Stdout("Parsing ")(name)(" over ")(inp[start..$].length).newline;
        // First, collect any left recursions
        if (lRecursions.length == 0 && sub.length != 0) {
            lRecursions.length = sub.length;
            lRecursions[] = null;

            foreach (i, sge; sub) {
                if (sge.length > 0 && sge[0] is this) {
                    // Left recursive, mark it
                    Production npr = new Production(_name);
                    npr.addChoice(sge[1..$]);
                    lRecursions[i] = npr;
                }
            }
        }

        // Initialize the output
        ParseResultArr* prFinal = ParseResultArr.allocate();
        prFinal.gc.blessUp();

        // Bail out if we have no choices
        if (sub.length == 0) return prFinal;

        // Now find a choice that works and isn't left recursive
        foreach (ch, sge; sub) {
            if (sge.length > 0 && sge[0] is this) continue;

            // initialize our potential result
            ParseResultArr* prs = ParseResultArr.allocate();
            prs.gc.blessUp();

            ParseResult* basepr = ParseResult.allocate(this);
            basepr.choice = ch;
            basepr.consumed = null;
            basepr.remaining = inp[start..$];
            basepr.file = file;
            basepr.sline = line;
            basepr.eline = line;
            basepr.scol = col;
            basepr.ecol = col;
            prs.append(basepr);

            // consume each part
            foreach (elem; sge) {
                ParseResultArr* sprs = ParseResultArr.allocate();
                sprs.gc.blessUp();

                foreach (pr; prs.arr) {
                    ParseResultArr* eprs = elem.parse(inp, start + pr.consumed.length,
                                                      pr.file, pr.eline, pr.ecol);

                    foreach (epr; eprs.arr) {
                        // worked, so consume this
                        ParseResult* spr = ParseResult.allocate(pr);
                        spr.subResults.append(epr);
                        spr.consumed = inp[0..(pr.consumed.length + epr.consumed.length)];
                        spr.remaining = epr.remaining;
                        spr.eline = epr.eline;
                        spr.ecol = epr.ecol;
                        sprs.append(spr);
                    }

                    eprs.gc.blessDown();
                }

                prs.gc.blessDown();
                prs = sprs;
            }

            // now append valid results to the final result
            foreach (pr; prs.arr) {
                prFinal.append(pr);
            }
            prs.gc.blessDown();
        }
        //Stdout(name)(": ")(sub.length).newline;
        if (prFinal.arr.length == 0) return prFinal;

        // now check for left recursion
        for (int pri = 0; pri < prFinal.arr.length; pri++) {
            ParseResult* pr = prFinal.arr[pri];
            
            foreach (ch, lr; lRecursions) {
                if (lr is null) continue;
                ParseResultArr* sprs = lr.parse(inp, start + pr.consumed.length,
                                                pr.file, pr.eline, pr.ecol);

                foreach (spr; sprs.arr) {
                    if (spr.consumed.length == 0) {
                        // ignore it if it didn't consume anything
                        break;
                    }

                    // A recursion, wrap it up
                    ParseResult* npr = ParseResult.allocate(this);
                    npr.choice = ch;
                    npr.subResults.append(pr);
                    foreach (sspr; spr.subResults.arr) {
                        npr.subResults.append(sspr);
                    }
                    npr.consumed = inp[0..(pr.consumed.length + spr.consumed.length)];
                    npr.remaining = spr.remaining;
                    npr.file = file;
                    npr.sline = pr.sline;
                    npr.scol = pr.scol;
                    npr.eline = spr.eline;
                    npr.ecol = spr.ecol;
                    prFinal.append(npr);
                }

                sprs.gc.blessDown();
            }
        }

        //Stdout("Parsed ")(name).newline;
        return prFinal;
    }

    private:

    /** The sub-elements of this production */
    GrammarElem[][] sub;

    /** Left-recursive postfixes */
    Production[] lRecursions = null;
}

/** A terminal, which is implemented with PCRE */
class RegexTerminal : GrammarElem {
    /** Create given a string regex */
    this(char[] sname, char[] sregex) {
        _name = sname.dup;

        char* err;
        int erroffset;

        _regex = pcre_compile(
                (sregex~"\0").ptr,
                PCRE_DOTALL,
                &err, &erroffset,
                null);
        if (_regex is null) {
            throw new RegexCompilationException(fromStringz(err));
        }
    }

    /** Parse this regex */
    ParseResultArr* gparse(char[] inp, uint start, char[] file, uint line, uint col)
    {
        ParseResultArr* ret = ParseResultArr.allocate();
        ret.gc.blessUp();

        int ovector[32];

        int result = pcre_exec(
                _regex, null,
                inp.ptr + start, inp.length - start, 0,
                PCRE_ANCHORED,
                ovector.ptr, 32);

        //Stdout("Parsing Regex ")(name).newline;
        //if (result <= 0) Stdout("FAIL").newline;
        if (result <= 0) return ret;
        //Stdout("PASS ");

        // didn't fail, so create a parseresult
        int len;
        if (result >= 2) {
            // captured a substring, use that
            len = ovector[3];
        } else {
            len = ovector[1];
        }
        //Stdout(" with ")(len).newline;
        ParseResult* pr = ParseResult.allocate(this);
        pr.consumed = inp[start..start+len];
        pr.remaining = inp[start+len..$];

        // now figure out the lines and columns
        pr.file = file;
        pr.sline = line;
        pr.scol = col;

        foreach (ch; pr.consumed) {
            if (ch == '\n') {
                line++;
                col = 0;
            } else {
                col++;
            }
        }
        pr.eline = line;
        pr.ecol = col;

        ret.append(pr);

        return ret;
    }

    /** The regex itself */
    private pcre* _regex;
}

/** The exception thrown when a regex does not compile */
class RegexCompilationException : Exception {
    this(char[] msg) { super(msg); }
}
