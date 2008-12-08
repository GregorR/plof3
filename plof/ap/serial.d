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
import tango.core.sync.ReadWriteMutex;

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

    // accessors
    uint val() { return _val; }
    SID next() { return _next; }

    private {
        uint _val;
        SID _next;
        uint _depth;
    }
}

/// Serial-semantics-guaranteeing read or write of a single value, returning list of actions that need to be canceled
class SerialAccessor(Action, Obj) {
    this() {
        writeListLock = new ReadWriteMutex();
    }

    /// Write a value, returning the old value
    Obj write(Action act, Obj val) {
        // Can't cancel while locked, as that could cause deadlock
        Action[] toCancel;
        Obj last;

        writeListLock.writer.lock();

            // set up our own writer
            Writer w;
            w.act = act;
            w.value = val;

            // now set up all the writer's read lists
            w.initReadLists(act);

            // find the appropriate place
            size_t wloc = ubound(writeList, w);

            if (wloc > 0) {
                // we may be cancelling something, check the last writer
                Writer lastw = writeList[wloc-1];
                last = lastw.value;

                // cancel in each read list
                for (int i = 0; i < lastw.readList.length; i++) {
                    lastw.readListLock[i].writer.lock();

                        Action[] readList = lastw.readList[i];

                        // figure out the first to be canceled
                        size_t rloc = ubound(readList, act);
                        if (rloc < readList.length) {
                            // if the data hasn't changed, just move them. Otherwise cancel them
                            if (last is val) {
                                w.readList[i] = readList[rloc..$].dup;
                            } else {
                                // cancel them
                                foreach (cact; readList[rloc..$])
                                    toCancel ~= cact;
                            }
                            readList.length = rloc;
                            lastw.readList[i] = readList;
                        }

                    lastw.readListLock[i].writer.unlock();
                }
            }


            // insert the writer (somewhat efficiently)
            writeList.length = writeList.length + 1;
            for (int i = writeList.length - 1; i > wloc; i--) {
                writeList[i] = writeList[i-1];
            }
            writeList[wloc] = w;
        
        writeListLock.writer.unlock();

        // add the actions
        act.addUndo(&undo);
        act.addCommit(&commit);

        // Now cancel anything that needs to be
        foreach (cact; toCancel)
            cact.cancel();

        return last;
    }

    /// Undo all writes from a given action
    void undo(Action act) {
        Action[] toCancel;

        writeListLock.writer.lock();

            // set up our own pseudo-writer
            Writer w;
            w.act = act;
            Obj prevval, curval;

            // find the first owned write
            size_t first = lbound(writeList, w);
            if (first == writeList.length) {
                writeListLock.writer.unlock();
                return; // nothing to undo
            }

            // get the previous value
            if (first > 0) {
                prevval = writeList[first-1].value;
            }
            curval = writeList[first].value;

            // find the last owned write
            int last;
            for (last = first; last < writeList.length && writeList[last].act == act; last++) {}

            // cancel them
            for (int i = first; i < last; i++) {
                for (int j = 0; j < writeList[i].readList.length; j++) {
                    writeList[i].readListLock[j].writer.lock();

                    // cancel or copy them
                    if (first > 0 && prevval is curval) {
                        writeList[first-1].readListLock[j].writer.lock(); // FIXME: blech
                        writeList[first-1].readList[j] ~= writeList[i].readList[j];
                        writeList[first-1].readListLock[j].writer.unlock();

                    } else {
                        foreach (cact; writeList[i].readList[j]) {
                            toCancel ~= cact;
                        }
                    }

                    writeList[i].readListLock[j].writer.unlock();
                }
            }
            
            // then remove the writers
            if (first != last) {
                int diff = last - first;
                for (int i = first; i < writeList.length - diff && i < last; i++) {
                    writeList[i] = writeList[i+diff];
                }
                writeList.length = writeList.length - diff;
            }

        writeListLock.writer.unlock();

        // now cancel them
        foreach (cact; toCancel) {
            cact.cancel();
        }
    }

    /// Commit a write, removing any previous writes
    void commit(Action act) {
        writeListLock.writer.lock();

            // set up a pseudo-writer
            Writer w;
            w.act = act;

            // find the last committable write
            ptrdiff_t last = ubound(writeList, w) - 1;
            if (last < 1) {
                writeListLock.writer.unlock();
                return; // nothing to remove
            }

            // and remove the earlier ones
            for (int i = 0; i < writeList.length - last; i++) {
                writeList[i] = writeList[i+last];
            }
            writeList.length = writeList.length - last;

        writeListLock.writer.unlock();
    }

    /// Read a value
    Obj read(Action act) {
        // Our reader/writer lock can change, so keep track of which needs to be unlocked
        void delegate() tounlock = &writeListLock.reader.unlock;

        Obj val;

        writeListLock.reader.lock();

            // make an equivalent writer to get the location
            Writer w;
            w.act = act;

            // find the appropriate writer
            ptrdiff_t wloc = (cast(ptrdiff_t) ubound(writeList, w)) - 1;

            if (wloc == -1) {
                // There is no writer! Probably need to invent one
                writeListLock.reader.unlock();
                writeListLock.writer.lock();
                tounlock = &writeListLock.writer.unlock;

                // check again
                wloc = (cast(ptrdiff_t) ubound(writeList, w)) - 1;

                if (wloc == -1) {
                    // OK, definitely need to invent one
                    w.act = act.gctx.initAction;
                    w.initReadLists(act);

                    // insert the writer (efficiently)
                    writeList.length = writeList.length + 1;
                    for (int i = writeList.length - 1; i > 0; i--) {
                        writeList[i] = writeList[i-1];
                    }
                    writeList[0] = w;

                    wloc = 0;
                }
            }

            // OK, get the appropriate writer structure
            Writer* lastw = &(writeList[wloc]);

            // figure out which read list we belong to
            uint tnum = act.gctx.tp.getThis();
            lastw.readListLock[tnum].writer.lock();

                Action[] readList = lastw.readList[tnum];

                size_t rloc = ubound(readList, act);

                // insert the reader (efficiently)
                readList.length = readList.length + 1;
                for (int i = readList.length - 1; i > rloc; i--) {
                    readList[i] = readList[i-1];
                }
                readList[rloc] = act;

                lastw.readList[tnum] = readList;

            lastw.readListLock[tnum].writer.unlock();

            val = lastw.value;
        tounlock();

        return val;
    }

    /// A writer has a value as well as a list of readers
    private struct Writer {
        Action act;
        Obj value;

        /// Initialize the read lists and locks (necessary for all read writers)
        void initReadLists(Action act) {
            uint tc = act.gctx.tp.threadCount();
            readListLock.length = tc;
            foreach (i, _; readListLock)
                readListLock[i] = new ReadWriteMutex();
            readList.length = tc;
        }

        ReadWriteMutex[] readListLock;
        Action[][] readList;

        int opCmp(Writer r) { return act.opCmp(r.act); }
    }

    private ReadWriteMutex writeListLock;
    private Writer[] writeList;
}
