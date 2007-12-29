/**
 * Base definition for PSL objects
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

module plof.psl.pslobject;

import tango.stdc.stdlib;

import plof.psl.treemap;
import plof.psl.gc;

/// A PSL object
struct PSLObject {
    PlofGCable gc;

    /// Allocate a PSLObject with the given parent
    static PSLObject* allocate(PSLObject* parent)
    {
        PSLObject* ret = PlofGCable.allocate!(PSLObject)();

        // Needs a foreachRef function for mark()
        ret.gc.foreachRef = &ret.foreachRef;

        // The parent may be null right now
        if (parent !is null) {
            ret.parent = parent;
        }

        // And needs a treemap of members
        ret.members = PlofTreemap.allocate();
        ret.members.gc.refUp();

        return ret;
    }

    /// Foreach over references
    void foreachRef(void delegate(PlofGCable*) callback)
    {
        if (_parent !is null) {
            callback(cast(PlofGCable*) _parent);
        }

        callback(cast(PlofGCable*) members);

        if (isArray) {
            callback(cast(PlofGCable*) arr);
        } else if (raw !is null) {
            callback(cast(PlofGCable*) raw);
        }
    }

    /// Parent of this object
    PSLObject* parent() { return _parent; }
    /// ditto
    void parent(PSLObject* sparent)
    {
        PSLObject* lastparent = _parent;
        _parent = sparent;
        if (_parent !is null) _parent.gc.refUp();
        if (lastparent !is null) lastparent.gc.refDown();
    }
    /// ditto
    private PSLObject* _parent;

    /// Members
    PlofTreemap* members;

    /// Array data
    bool isArray() { return _isArray; }
    /// ditto
    PSLArray* arr() { return _arr; }
    /// ditto
    void arr(PSLArray* sarr)
    {
        if (_isArray) {
            // replace old array
            PSLArray* oldarr = _arr;
            _arr = sarr;
            if (sarr !is null) sarr.gc.refUp();
            if (oldarr !is null) oldarr.gc.refDown();

        } else {
            // replace raw data with array data
            PSLRawData* oldraw = _raw;
            _isArray = true;
            _arr = sarr;
            if (sarr !is null) sarr.gc.refUp();
            if (oldraw !is null) oldraw.gc.refDown();

        }
    }
    
    /// Raw data
    PSLRawData* raw() { return _raw; }
    /// ditto
    void raw(PSLRawData* sraw)
    {
        if (_isArray) {
            // replace array data with raw data
            PSLArray* oldarr = _arr;
            _isArray = false;
            _raw = sraw;
            if (sraw !is null) sraw.gc.refUp();
            if (oldarr !is null) oldarr.gc.refDown();

        } else {
            // replace old raw data with new raw data
            PSLRawData* oldraw = _raw;
            _raw = sraw;
            if (sraw !is null) sraw.gc.refUp();
            if (oldraw !is null) oldraw.gc.refDown();

        }
    }

    private {
        bool _isArray;
        union {
            PSLRawData* _raw;
            PSLArray* _arr;
        }
    }
}

/// Raw data
struct PSLRawData {
    PlofGCable gc;

    /// Allocate, given the provided raw data
    static PSLRawData* allocate(ubyte[] data)
    {
        PSLRawData* ret = PlofGCable.allocate!(PSLRawData)();
        ret.gc.destructor = &ret.destructor;

        // use malloc to get the data, to avoid D's GC
        if (data.length != 0) {
            ret.data = (cast(ubyte*) malloc(data.length))[0..data.length];
            ret.data[] = data[];
        }

        return ret;
    }

    /// Allocate, concatenating two raw datas
    static PSLRawData* allocate(ubyte[] left, ubyte[] right)
    {
        PSLRawData* ret = PlofGCable.allocate!(PSLRawData)();
        ret.gc.destructor = &ret.destructor;

        // no D GC
        if (left.length + right.length != 0) {
            ret.data = (cast(ubyte*) malloc(left.length + right.length))
                [0..(left.length + right.length)];
            ret.data[0..left.length] = left[];
            ret.data[left.length..$] = right[];
        }

        return ret;
    }

    /// The destructor, to free the underlying data
    void destructor()
    {
        if (data.ptr !is null) free(data.ptr);
    }

    /// The data itself
    ubyte[] data;
}

/// A PSL array
struct PSLArray {
    PlofGCable gc;

    /// Allocate, given the provided array
    static PSLArray* allocate(PSLObject*[] arr)
    {
        PSLArray* ret = PlofGCable.allocate!(PSLArray)();
        ret.gc.foreachRef = &ret.foreachRef;
        ret.gc.destructor = &ret.destructor;

        // use malloc to get the space, to avoid D's GC
        if (arr.length != 0) {
            ret.arr = (cast(PSLObject**) malloc(arr.length * (PSLObject*).sizeof))[0..arr.length];
            ret.arr[] = arr[];

            // now up all the reference counts
            foreach (i; arr) {
                i.gc.refUp();
            }
        }

        return ret;
    }

    /// Allocate, concatenating the two provided arrays
    static PSLArray* allocate(PSLObject*[] left, PSLObject*[] right)
    {
        PSLArray* ret = PlofGCable.allocate!(PSLArray)();
        ret.gc.foreachRef = &ret.foreachRef;
        ret.gc.destructor = &ret.destructor;

        // use malloc to get the space, to avoid D's GC
        if (left.length + right.length != 0) {
            ret.arr = (cast(PSLObject**) malloc((left.length + right.length) *
                                                (PSLObject*).sizeof))
                [0..(left.length + right.length)];
            ret.arr[0..left.length] = left[];
            ret.arr[left.length..$] = right[];

            // now up all the reference counts
            foreach (i; ret.arr) {
                i.gc.refUp();
            }
        }

        return ret;
    }

    /// Foreach over references
    void foreachRef(void delegate(PlofGCable*) callback)
    {
        foreach (i; arr) {
            callback(cast(PlofGCable*) i);
        }
    }

    /// Get rid of the data used to store the array itself
    void destructor()
    {
        if (arr.ptr !is null) free(arr.ptr);
    }

    // (Re)set the length of this array
    void setLength(size_t len)
    {
        if (len > arr.length) {
            // need to reallocate
            int origlen = arr.length;
            arr = (cast(PSLObject**) realloc(arr.ptr,
                len * (PSLObject*).sizeof))[0..len];

            // set the remainder to pslNull
            arr[origlen..$] = pslNull;
            for (int i = origlen; i < arr.length; i++)
                pslNull.gc.refUp();
            
        } else {
            // deref
            foreach (e; arr[len..$]) {
                e.gc.refDown();
            }

            // length <=, so just resize
            arr = arr[0..len];
        }
    }

    // The array itself
    PSLObject*[] arr;
}

// Global objects
PSLObject* pslNull, pslGlobal;
static this() {
    pslNull = PSLObject.allocate(null);
    pslNull.gc.blessUp();

    pslNull.parent = pslNull;
    pslNull.gc.refUp();

    pslGlobal = PSLObject.allocate(pslNull);
    pslGlobal.gc.blessUp();
}
