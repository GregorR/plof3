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

import plof.ap.serial;

import plof.ast.ast;

/// Serial accessor for objects
alias SerialAccessor!(Action, APObject) APAccessor;

/// Automatic parallelization object
class APObject {
    /// Allocate a PSLObject with the given parent by the given action
    this(Action act, APObject parent)
    {
        this();
        _parent.write(act, parent);
    }

    /// Allocate a PSLObject with no parent (should only be used once, for the null object)
    this() {
        _parent = new APAccessor();
    }

    /// Parent of this object
    APObject getParent(Action act) {
        return _parent.read(act);
    }
    /// ditto
    Action[] setParent(Action act, APObject sparent)
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
    ubyte[] getRaw(Action act) {
        return _raw.read(act);
    }
    /// ditto
    Action[] setRaw(Action act, ubyte[] sraw)
    {
        return _raw.write(act, sraw);
    }

    /// Function data
    PASTNode getAST(Action act) {
        return _ast.read(act);
    }
    /// ditto
    Action[] setAST(Action act, PASTNode ast) {
        return _ast.write(act, ast);
    }

    private {
        /*bool _isArray;
        union {
            PSLRawData _raw;
            PSLArray _arr;
        }*/
        SerialAccessor!(Action, ubyte[]) _raw;
        SerialAccessor!(Action, PASTNode) _ast;
    }
}

/// AP actions are really just a serialization ID associated with an AST node to execute
class Action {
    this(SID sid, PASTNode ast, APAccessor ctx, APAccessor args, APAccessor[] temps) {
        _sid = sid;
        _ast = ast;
        _ctx = ctx;
        _args = args;
        _temps = temps;
    }

    // compare by the SID
    int opCmp(Action r) { return _sid.opCmp(r._sid); }
    bool opEquals(Action r) { return _sid.opEquals(r._sid); }

    SID sid() { return _sid; }
    PASTNode ast() { return _ast; }
    APAccessor ctx() { return _ctx; }
    APAccessor args() { return _args; }
    APAccessor[] temps() { return _temps; }

    private {
        SID _sid;
        PASTNode _ast;
        APAccessor _ctx, _args;
        APAccessor[] _temps;
    }
}
