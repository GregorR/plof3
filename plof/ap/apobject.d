/**
 * Object definition with automatic parallelization
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

module plof.ap.apobject;

import plof.ap.action;
import plof.ap.serial;

/// Serial accessor for objects
alias SerialAccessor!(Action, APObject) APAccessor;

class APObject {
    /// Allocate a PSLObject with the given parent by the given action
    this(Action act, APObject parent)
    {
        _parent = new APAccessor();
        _parent.write(act, parent);
    }

    /// Parent of this object
    APObject parent(Action act) {
        return _parent.read(act);
    }
    /// ditto
    Action[] parent(Action act, APObject sparent)
    {
        return _parent.write(act, sparent);
    }
    /// ditto
    private APAccessor _parent;

    /// Members
    APAccessor[char[]] members;

    /* FIXME: arrays
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
    } */
    
    /// Raw data
    ubyte[] raw(Action act) {
        return _raw.read(act);
    }
    /// ditto
    Action[] raw(Action act, ubyte[] sraw)
    {
        return _raw.write(act, sraw);
    }

    private {
        /*bool _isArray;
        union {
            PSLRawData _raw;
            PSLArray _arr;
        }*/
        SerialAccessor!(Action, ubyte[]) _raw;
    }
}
