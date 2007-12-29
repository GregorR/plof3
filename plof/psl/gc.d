/**
 * Garbage collector for the PSL interpreter
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

module plof.psl.gc;

import tango.stdc.stdlib;

// The PSL GC, which is a reference counter + mark-sweep

int gccount()
{
    int c;
    PlofGCable* cur;
    for (c = 0, cur = plofGChead;
         cur !is null;
         c++, cur = cur.next) {}
    return c;
}

/// A GC-able entity. This must be the first element of a GC-able struct
struct PlofGCable {
    /// Allocate a GCable struct of type T
    static T* allocate(T)()
    {
        plofGCcount++;

        // now allocate
        T* ret = cast(T*) calloc(T.sizeof, 1);

        // add it to the list
        if (plofGChead is null) {
            plofGChead = cast(PlofGCable*) ret;
            plofGCtail = cast(PlofGCable*) ret;

        } else {
            plofGCtail.next = cast(PlofGCable*) ret;
            (cast(PlofGCable*) ret).prev = plofGCtail;
            plofGCtail = cast(PlofGCable*) ret;

        }

        return ret;
    }

    /// Free this GCable object
    void free()
    {
        plofGCcount--;

        if (destructor !is null) {
            destructor();
        }

        // remove this from the GC list...
        if (prev is null) {
            plofGChead = next;
        } else {
            prev.next = next;
        }

        if (next is null) {
            plofGCtail = prev;
        } else {
            next.prev = prev;
        }

        // and free it
        tango.stdc.stdlib.free(this);
    }

    /// Increase the reference count of this object
    void refUp()
    {
        refc++;
    }

    /// Decrease the reference count of this object
    void refDown()
    {
        refc--;
        if (refc == 0) {
            // deref others
            if (foreachRef !is null) {
                foreachRef((PlofGCable* next) {
                    next.refDown();
                });
            }

            // if the reference count is <0, this is already being cleaned up
            free();
        }
    }

    /// Increase blessed references
    void blessUp()
    {
        refUp();
        bless++;
    }

    /// Decrease blessed references
    void blessDown()
    {
        bless--;
        refDown();
    }

    /// Perhaps run the GC
    static void maybeGC()
    {
        if (plofGCcount >= plofGClimit) {
            // run the GC
            gc();

            // and perhaps increase the limit
            if (plofGCcount > (plofGClimit >> 1)) {
                plofGClimit <<= 1;
            }
        }
    }

    /// Run the GC
    static void gc()
    {
        zero();
        markPhase();
        sweep();
    }

    /// Zero all reference counts (before a mark phase)
    static void zero()
    {
        PlofGCable* cur = plofGChead;
        while (cur !is null) {
            cur.refc = 0;
            cur = cur.next;
        }
    }

    /** Mark phase */
    static void markPhase()
    {
        for (PlofGCable* cur = plofGChead;
             cur !is null;
             cur = cur.next) {
            if (cur.bless) {
                if (cur.refc == 0) {
                    cur.mark();
                    cur.refc += cur.bless - 1;
                } else {
                    cur.refc += cur.bless;
                }
            }
        }
    }

    /** Mark phase starting with this object */
    void mark()
    {
        refc++;
        if (foreachRef !is null) {
            foreachRef((PlofGCable* next) {
                if (next.refc == 0) {
                    // first time this has been referenced, so mark it
                    next.mark();
                } else {
                    next.refc++;
                }
            });
        }
    }

    /** Sweep phase */
    static void sweep()
    {
        PlofGCable* cur = plofGChead;
        PlofGCable* next;
        while (cur !is null) {
            next = cur.next;
            if (cur.refc == 0) {
                // no real references, so free
                cur.free();
            }
            cur = next;
        }
    }

    /// Total number of things allocated since the last GC
    static int acount;

    /** The current reference count */
    int refc;

    /** Amount of blessed references */
    int bless;

    /** The delegate which will foreach over referenced objects */
    void delegate(void delegate(PlofGCable*)) foreachRef;

    /** A destructor, if necessary */
    void delegate() destructor;

    /** The next and previous elements, for the sweep phase */
    PlofGCable* next, prev;
}

private:

// The Plof GC LLL
PlofGCable* plofGChead = null;
PlofGCable* plofGCtail = null;

// The total number of GC'd entities
int plofGCcount = 0;

// The limit before GC'ing
int plofGClimit = 128;
