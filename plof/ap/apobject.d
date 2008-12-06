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

import tango.core.sync.ReadWriteMutex;

import plof.ap.serial;

import plof.ast.ast;

/// Serial accessor for objects
alias SerialAccessor!(Action, APObject) APAccessor;

/// Automatic parallelization object
class APObject {
    /// Allocate a PSLObject with the given parent by the given action
    this(Action act, APObject parent) {
        this();
        _parent.write(act, parent);
    }

    /// Allocate a PSLObject with no parent (should only be used once, for the null object)
    this() {
        _parent = new APAccessor();
        _memberLock = new ReadWriteMutex();
        _raw = new SerialAccessor!(Action, ubyte[])();
        _ast = new SerialAccessor!(Action, PASTNode)();
    }


    /// Parent of this object
    APObject getParent(Action act) {
        return _parent.read(act);
    }
    /// ditto
    void setParent(Action act, APObject sparent) {
        _parent.write(act, sparent);
    }
    /// ditto


    /// Internal functino to get a member accessor
    private APAccessor getMemberAccessor(ubyte[] name) {
        APAccessor ret;
        APAccessor* pret;

        // First hope it's there with a read lock
        _memberLock.reader.lock();
        pret = name in _members;
        if (pret !is null) ret = *pret;
        _memberLock.reader.unlock();

        // If it wasn't, need to create it
        if (pret is null) {
            _memberLock.writer.lock();

            // but somebody may have created it in the interim
            pret = name in _members;
            if (pret is null) {
                _members[name] = ret = new APAccessor();
            } else {
                ret = *pret;
            }

            _memberLock.writer.unlock();
        }

        return ret;
    }

    /// Get a member
    APObject getMember(Action act, ubyte[] name) {
        APAccessor acc = getMemberAccessor(name);
        return acc.read(act);
    }
    /// Set a member
    void setMember(Action act, ubyte[] name, APObject to) {
        APAccessor acc = getMemberAccessor(name);
        acc.write(act, to);
    }

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
    void setRaw(Action act, ubyte[] sraw) {
        _raw.write(act, sraw);
    }

    /// Function data
    PASTNode getAST(Action act) {
        return _ast.read(act);
    }
    /// ditto
    void setAST(Action act, PASTNode ast) {
        _ast.write(act, ast);
    }

    private {
        /*bool _isArray;
        union {
            PSLRawData _raw;
            PSLArray _arr;
        }*/
        APAccessor _parent;
        APAccessor[ubyte[]] _members;
        ReadWriteMutex _memberLock;
        SerialAccessor!(Action, ubyte[]) _raw;
        SerialAccessor!(Action, PASTNode) _ast;
    }
}

/// AP actions are really just a serialization ID associated with an AST node to execute
class Action {
    this(SID sid, APGlobalContext gctx, PASTNode ast, APObject ctx, APObject arg, APAccessor[] temps) {
        _sid = sid;
        _gctx = gctx;
        _ast = ast;
        _ctx = ctx;
        _arg = arg;
        _temps = temps;
    }

    /// Cancel this action
    void cancel() {
        // to be implemented
    }

    // compare by the SID
    int opCmp(Action r) { return _sid.opCmp(r._sid); }
    bool opEquals(Action r) { return _sid.opEquals(r._sid); }

    // accessors
    SID sid() { return _sid; }
    APGlobalContext gctx() { return _gctx; }
    PASTNode ast() { return _ast; }
    APObject ctx() { return _ctx; }
    APObject arg() { return _arg; }
    APAccessor[] temps() { return _temps; }

    private {
        SID _sid;
        APGlobalContext _gctx;
        PASTNode _ast;
        APObject _ctx, _arg;
        APAccessor[] _temps;
    }
}

/// Global execution context of Plof
class APGlobalContext {
    this() {
        nul = new APObject();
        initAction = new Action(new SID(0, null), this, null, nul, nul, []);
        nul.setParent(initAction, nul);
        global = new APObject(initAction, nul);
    }

    Action initAction;
    APObject nul, global;
}
