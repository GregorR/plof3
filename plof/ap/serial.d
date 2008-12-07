/**
 * Serialization constructs for automatic parallelization
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

module plof.ap.serial;

import tango.core.Array;

import tango.io.Stdout;

/// Serialization ID
class SID {
    this(uint val, SID next) {
        _val = val;
        _next = next;
        
        if (next is null) {
            _depth = 0;
        } else {
            _depth = next._depth + 1;
        }
    }

    /*void debugOut() {
        Stdout(_val);
        if (_next !is null) {
            Stdout(":");
            _next.debugOut();
        }
    }*/

    /// Can compare to other SIDs, but that's all
    int opCmp(SID r) {
        SID l = this;
        int pref = 0; /// Preference towards left or right for depth mismatch

        // make the depth the same
        while (l._depth > r._depth) {
            l = l._next;
            pref = 1;
        }
        while (r._depth > l._depth) {
            r = r._next;
            pref = -1;
        }

        // find the last nonmatching
        while (l !is null && l._next !is r._next) {
            l = l._next;
            r = r._next;
        }

        // if they were equal from here
recurse:
        if (l is r) {
            // then it's the preferred side
            return pref;

        } else {
            // otherwise it's whichever side is greater
            if (l._val < r._val) {
                return -1;
            } else if (l._val > r._val) {
                return 1;
            } else {
                // they're equal, recurse
                l = l._next;
                r = r._next;
                goto recurse;
            }
        }
    }

    /// Equality is just opCmp
    bool opEquals(SID r) {
        return opCmp(r) == 0;
    }

    // accessor
    uint val() { return _val; }

    private {
        uint _val;
        SID _next;
        uint _depth;
    }
}

/// Serial-semantics-guaranteeing read or write of a single value, returning list of actions that need to be canceled
class SerialAccessor(Action, Obj) {
    /// Write a value, returning the old value
    Obj write(Action act, Obj val) {
        synchronized (this) {
            Obj last;

            // set up our own writer
            Writer w;
            w.act = act;
            w.value = val;

            // find the appropriate place
            size_t wloc = ubound(writeList, w);

            if (wloc > 0) {
                // we may be cancelling something, check the last writer
                Writer lastw = writeList[wloc-1];
                last = lastw.value;

                // look for the first reader that would need to be cancelled
                size_t rloc = ubound(lastw.readList, act);
                if (rloc < lastw.readList.length) {
                    // cancel them
                    foreach (cact; lastw.readList[rloc..$])
                        cact.cancel();
                    lastw.readList.length = rloc;
                }
            }

            // insert the writer (efficiently)
            writeList.length = writeList.length + 1;
            for (int i = writeList.length - 1; i > wloc; i--) {
                writeList[i] = writeList[i-1];
            }
            writeList[wloc] = w;

            // and add the undo action
            act.addUndo(&undo);

            return last;
        }
    }

    /// Undo all writes from a given action
    void undo(Action act) {
        synchronized (this) {
            // set up our own writer
            Writer w;
            w.act = act;

            // find the first owned write
            size_t first = lbound(writeList, w);
            if (first == writeList.length) {
                return; // nothing to undo
            }

            // find the last owned write
            int last;
            for (last = first; last < writeList.length && writeList[last].act == act; last++) {}

            // cancel them
            for (int i = first; i < last; i++) {
                foreach (cact; writeList[i].readList) {
                    cact.cancel();
                }
            }
            
            // then remove the writers
            if (first != last) {
                int diff = last - first;
                for (int i = first; i < last; i++) {
                    writeList[i] = writeList[i+diff];
                }
                writeList.length = writeList.length - diff;
            }
        }
    }

    /// Read a value
    Obj read(Action act) {
        synchronized (this) {
            // make and equivalent writer to get the location
            Writer w;
            w.act = act;

            // find the appropriate writer
            ptrdiff_t wloc = (cast(ptrdiff_t) ubound(writeList, w)) - 1;

            if (wloc == -1) {
                // There is no writer! Need to invent one
                w.act = act.gctx.initAction;

                // insert the writer (efficiently)
                writeList.length = writeList.length + 1;
                for (int i = writeList.length - 1; i > 0; i--) {
                    writeList[i] = writeList[i-1];
                }
                writeList[0] = w;

                wloc = 0;
            }

            // add ourself to the readlist
            Writer* lastw = &(writeList[wloc]);

            size_t rloc = ubound(lastw.readList, act);

            // insert the reader (efficiently)
            lastw.readList.length = lastw.readList.length + 1;
            for (int i = lastw.readList.length - 1; i > rloc; i--) {
                lastw.readList[i] = lastw.readList[i-1];
            }
            lastw.readList[rloc] = act;

            return lastw.value;
        }
    }

    /// A writer has a value as well as a list of readers
    private struct Writer {
        Action act;
        Obj value;
        Action[] readList;

        int opCmp(Writer r) { return act.opCmp(r.act); }
    }

    private Writer[] writeList;
}
