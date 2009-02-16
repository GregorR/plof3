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

/// A PSL object
class PSLObject {
    /// Allocate a PSLObject with the given parent
    this(PSLObject parent)
    {
        // The parent may be null right now
        if (parent !is null) {
            this.parent = parent;
        }

        // And needs a treemap of members
        this.members = new PlofTreemap;
    }

    /// Parent of this object
    PSLObject parent() { return _parent; }
    /// ditto
    void parent(PSLObject sparent)
    {
        _parent = sparent;
    }
    /// ditto
    private PSLObject _parent;

    /// Members
    PlofTreemap members;

    /// Array data
    bool isArray() { return _isArray; }
    /// ditto
    PSLArray arr() { return _arr; }
    /// ditto
    void arr(PSLArray sarr)
    {
        if (_isArray) {
            // replace old array
            _arr = sarr;

        } else {
            // replace raw data with array data
            _isArray = true;
            _arr = sarr;

        }
    }
    
    /// Raw data
    PSLRawData raw() { return _raw; }
    /// ditto
    void raw(PSLRawData sraw)
    {
        if (_isArray) {
            // replace array data with raw data
            _isArray = false;
            _raw = sraw;

        } else {
            // replace old raw data with new raw data
            _raw = sraw;

        }
    }

    private {
        bool _isArray;
        union {
            PSLRawData _raw;
            PSLArray _arr;
        }
    }
}

/// Raw data
class PSLRawData {
    /// Allocate, given the provided raw data
    this(ubyte[] data)
    {
        // get the data
        if (data.length != 0) {
            this.data = new ubyte[data.length];
            this.data[] = data[];
        }
    }

    /// Allocate, concatenating two raw datas
    this(ubyte[] left, ubyte[] right)
    {
        if (left.length + right.length != 0) {
            this.data = new ubyte[left.length + right.length];
            this.data[0..left.length] = left[];
            this.data[left.length..$] = right[];
        }
    }

    /// The data itself
    ubyte[] data;
}

/// A PSL array
class PSLArray {
    /// Allocate, given the provided array
    this(PSLObject[] arr)
    {
        if (arr.length != 0) {
            this.arr = new PSLObject[arr.length];
            this.arr[] = arr[];
        }
    }

    /// Allocate, concatenating the two provided arrays
    this(PSLObject[] left, PSLObject[] right)
    {
        if (left.length + right.length != 0) {
            this.arr = new PSLObject[left.length + right.length];
            this.arr[0..left.length] = left[];
            this.arr[left.length..$] = right[];
        }
    }

    // (Re)set the length of this array
    void setLength(size_t len)
    {
        if (len > arr.length) {
            // need to reallocate
            int origlen = arr.length;
            arr.length = len;

            // set the remainder to pslNull
            arr[origlen..$] = pslNull;
        } else {
            // length <=, so just resize
            arr.length = len;
        }
    }

    // The array itself
    PSLObject[] arr;
}

// Global objects
PSLObject pslNull, pslGlobal;
static this() {
    pslNull = new PSLObject(null);
    pslNull.parent = pslNull;

    pslGlobal = new PSLObject(pslNull);
}
