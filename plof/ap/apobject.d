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

import tango.core.sync.Condition;
import tango.core.sync.Mutex;
import tango.core.sync.ReadWriteMutex;

import tango.io.Stdout;

import plof.ap.serial;

// FIXME: recursive dependencies
import plof.ap.interp;
import plof.ap.threads;

import plof.ast.ast;

import std.c.stdlib;

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
        _memberLength = new SerialAccessor!(Action, Action)();
        _memberLock = new ReadWriteMutex();
        _arrayLength = new SerialAccessor!(Action, uint)();
        _arrayLock = new ReadWriteMutex();
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


    /// Internal function to get a member accessor
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
    /// Get the list of all members
    ubyte[][] getMembers(Action act) {
        // first make sure we'll get cancelled if the length changes
        _memberLength.read(act);

        // now just go through the array
        ubyte[][] names;
        _memberLock.reader.lock();
        foreach (name, member; _members)  {
            if (member.read(act) !is null) {
                names ~= name;
            }
        }
        _memberLock.reader.unlock();

        return names;
    }
    /// Set a member
    void setMember(Action act, ubyte[] name, APObject to) {
        APAccessor acc = getMemberAccessor(name);
        APObject oldval = acc.write(act, to);
        if (oldval is null) {
            // we changed the number of members
            _memberLength.write(act, act);
        }
    }


    /// Get the length of array data
    uint getArrayLength(Action act) {
        return _arrayLength.read(act);
    }
    /// Set the length of array data
    void setArrayLength(Action act, uint len) {
        // make sure it's actually long enough
        _arrayLock.reader.lock();
        uint origlen = _array.length;
        _arrayLock.reader.unlock();

        if (origlen < len) {
            _arrayLock.writer.lock();

            origlen = _array.length;
            if (origlen < len) {
                _array.length = len;

                // now initialize the rest
                for (uint i = origlen; i < len; i++) {
                    _array[i] = new APAccessor();
                }
            }

            _arrayLock.writer.unlock();
        }

        // then set the length
        _arrayLength.write(act, len);
    }

    /// If the array length has been set, this is an array
    bool isArray(Action act) {
        return getArrayLength(act) > 0;
    }

    /// Get an element of array data
    APObject getArrayElement(Action act, uint elem) {
        uint len = getArrayLength(act);

        // make sure we had this element at the time
        if (elem >= len) {
            return null;
        }

        // get the value
        _arrayLock.reader.lock();
        APObject ret = _array[elem].read(act);
        _arrayLock.reader.unlock();
        return ret;
    }
    /// Set an element of array data
    void setArrayElement(Action act, uint elem, APObject val) {
        uint len = getArrayLength(act);

        // if it's too short, bail out (FIXME)
        if (elem >= len) {
            return;
        }

        // set the value
        /* note that although this is a writer on the ELEMENT of the array,
         * it's a reader on the array OBJECT */
        _arrayLock.reader.lock();
        _array[elem].write(act, val);
        _arrayLock.reader.unlock();
    }

    
    /// Raw data
    ubyte[] getRaw(Action act) {
        return _raw.read(act);
    }
    /// ditto
    void setRaw(Action act, ubyte[] sraw) {
        _raw.write(act, sraw);
    }


    /// Integer data
    ptrdiff_t getInteger(Action act) {
        ubyte[] raw = _raw.read(act);

        // if it's the wrong length, garbage
        if (raw.length != ptrdiff_t.sizeof) {
            return 0;
        } else {
            return *(cast(ptrdiff_t*) raw.ptr);
        }
    }
    /// ditto
    void setInteger(Action act, ptrdiff_t sval) {
        setRaw(act, (cast(ubyte*) &sval)[0..ptrdiff_t.sizeof].dup);
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
        APAccessor _parent;

        // member data
        APAccessor[ubyte[]] _members;
        // we need to know when the /number/ of members has changed, so keep track of it
        SerialAccessor!(Action, Action) _memberLength;
        ReadWriteMutex _memberLock;

        // array data
        SerialAccessor!(Action, uint) _arrayLength;
        APAccessor[] _array;
        ReadWriteMutex _arrayLock;

        SerialAccessor!(Action, ubyte[]) _raw;
        SerialAccessor!(Action, PASTNode) _ast;
    }
}

/// AP action states
enum ActionState {
    None,
    Running,
    Canceled,
    Destroyed,
    Done,
    Committing,
    Committed
}

/// AP actions are really just a serialization ID associated with an AST node to execute
class Action {
    this(SID sid, APGlobalContext gctx, PASTNode ast, APObject ctx, APAccessor[] temps) {
        _sid = sid;
        _csid = new SID(0, _sid);
        _gctx = gctx;
        _ast = ast;
        _ctx = ctx;
        _temps = temps;
	_excptHandler = null;

        stateMutex = new Mutex();
        stateCondition = new Condition(stateMutex);
    }

    /// Create a child action
    Action createChild(PASTNode ast, APObject ctx, APAccessor[] temps, Action siblingof = null) {
        Action child;

        // only create children if we're in the right state
        stateMutex.lock();
            bool notdone = true;
            while (notdone) {
                switch (state) {
                    case ActionState.None:
                    case ActionState.Canceled:
                    case ActionState.Committing:
                        // these will eventually go
                        stateCondition.wait();
                        break;

                    case ActionState.Running:
                    case ActionState.Done:
                    case ActionState.Committed: // possible with loops
                        // these are OK
                        notdone = false;
                        break;

                    case ActionState.Destroyed:
                        // definitely can't make children
                        stateMutex.unlock();
                        return null;

                    default:
                        synchronized (Stderr)
                            Stderr("Action ")(ast.toXML())(" in bad state for spawning: ")(state).newline;
                }
            }

            synchronized (this) {
                // make sure the siblingof is sensible
                if (siblingof !is null &&
                    (siblingof.sid.next !is _csid || siblingof.sid.val >= _children.length)) {
                    // from a dead child
                    child = null;
                } else {
                    child = new Action(new SID(_children.length, _csid), _gctx, ast, ctx, temps);
                    child._parent = this;
                    _children ~= child;
                }
            }
        stateMutex.unlock();

        return child;
    }

    /// Create a sibling action (another child of the same parent)
    Action createSibling(PASTNode ast, APAccessor[] temps) {
        Action sibling;

        // only create siblings if we're in the right state
        stateMutex.lock();
            bool notdone = true;
            while (notdone) {
                switch (state) {
                    case ActionState.None:
                    case ActionState.Canceled:
                        // these will eventually go
                        stateCondition.wait();
                        break;

                    case ActionState.Running:
                    case ActionState.Done:
                    case ActionState.Committing:
                        // these are OK
                        notdone = false;
                        break;

                    case ActionState.Destroyed:
                        // definitely can't make children
                        stateMutex.unlock();
                        return null;

                    default:
                        synchronized (Stderr)
                            Stderr("Action ")(ast.toXML())(" in bad state for creating siblings: ")(state).newline;
                }
            }

            synchronized (this) {
                if (_parent is null) {
                    stateMutex.unlock();
                    throw new ActionCreationException("Cannot create sibling.");
                } else {
                    sibling = _parent.createChild(ast, _ctx, temps, this);
                }
            }
        stateMutex.unlock();

        return sibling;
    }

    /// Run this action
    void run() {
        // we're either just running or committing
        bool committing = false;
        stateMutex.lock();
            if (state == ActionState.Committing) committing = true;
        stateMutex.unlock();

        if (committing) {
            debugOut("committing.");

            // run all the commit actions
            foreach (commit; _commits) {
                commit(this);
            }

        } else {
            debugOut("running.");

            ast.accept(new APInterpVisitor(this));

        }
    }

    /// Cancel this action
    void cancel(bool fromRunning = false) {
        Action[] oldChildren;
        void delegate(Action)[] oldUndos;

        debugOut("canceled.");

        // wait for it to be done
        stateMutex.lock();
        bool notdone = true;
        while (notdone) {
            switch (state) {
                case ActionState.Running:
                    if (fromRunning) {
                        // we're allowed to cancel it while running (the running thread canceled itself)
                        notdone = false;
                        state = ActionState.Canceled;
                    } else {
                        stateCondition.wait();
                    }
                    break;

                case ActionState.None:
                case ActionState.Canceled:
                case ActionState.Destroyed:
                    // somebody else handled it
                    stateMutex.unlock();
                    return;

                case ActionState.Done:
                    notdone = false;
                    state = ActionState.Canceled;
                    break;

                default:
                    synchronized (Stderr)
                        Stderr("Action ")(ast.toXML())(" in bad state for cancelation: ")(state).newline;
            }
        }
        stateMutex.unlock();

        // re-enqueue this action
        synchronized (this) {
            // get the children ready for destruction
            oldChildren = _children;
            _children = null;
            _csid = new SID(_csid.val + 1, _sid);
            oldUndos = _undos;
            _undos = null;

            gctx.tp.enqueue([this]);
        }

        // destroy the children
        foreach (child; oldChildren) {
            child.destroy();
        }

        // undo
        foreach (f; oldUndos) {
            f(this);
        }

        // then tell other threads that it's ready to be run
        stateMutex.lock();
        switch (state) {
            case ActionState.Canceled:
                state = ActionState.None;
            case ActionState.Destroyed:
                stateCondition.notifyAll();
                break;

            default:
                synchronized (Stderr)
                    Stderr("Action ")(ast.toXML())(" in bad state for post-cancelation: ")(state).newline;
        }
        stateMutex.unlock();

    }

    /// Destroy this action
    void destroy() {
        Action[] oldChildren;
        void delegate(Action)[] oldUndos;

        debugOut("destroyed.");

        // wait for it to be done
        stateMutex.lock();
        bool notdone = true;
        while (notdone) {
            switch (state) {
                case ActionState.Running:
                    stateCondition.wait();
                    break;

                case ActionState.Destroyed:
                    // somebody else handled it
                    stateMutex.unlock();
                    return;

                case ActionState.None:
                case ActionState.Done:
                case ActionState.Canceled:
                    notdone = false;
                    state = ActionState.Destroyed;
                    stateCondition.notify();
                    break;

                default:
                    synchronized (Stderr)
                        Stderr("Action ")(ast.toXML())(" in bad state for destruction: ")(state).newline;
            }
        }
        stateMutex.unlock();

        synchronized (this) {
            // get the children ready for destruction
            oldChildren = _children;
            _children = null;
            _csid = new SID(_csid.val + 1, _sid);
            oldUndos = _undos;
            _undos = null;
        }

        // destroy the children
        foreach (child; oldChildren) {
            child.destroy();
        }

        // undo
        foreach (f; oldUndos) {
            f(this);
        }
    }

    /// Add something which must be undone if this action is destroyed
    void addUndo(void delegate(Action) f) {
        synchronized (this) {
            _undos ~= f;
        }
    }

    /// Add something which must be done when this action is committed
    void addCommit(void delegate(Action) f) {
        synchronized (this) {
            _commits ~= f;
        }
    }

    // Debug output
    version (ThreadDebug) {
        final void debugOut(char[] msg) {
            synchronized (Stderr) Stderr("Action ")(ast.toXML())(" ")(msg).newline;
        }
    } else {
        final void debugOut(char[] msg) {}
    }

    /// Find the exception handler corresponding to this action
    APObject findExcptHandler() {
        if (catches()) {
            return _excptHandler;
        } else if (_parent is null) {
            Stderr("Error: Uncaught exception").newline;
            exit(42);
        } else {
            return _parent.findExcptHandler();
        }
    }

    /// Do the cancels associated with a throw from this action
    void throwCancel(Action from = null) {
        if (from !is null) {
            // destroy the children
            Action[] oldChildren;
            synchronized (this) {
                if (from.sid.next !is _csid) {
                    // dead child trying to throw?
                    return;
                }

                oldChildren = _children[from.sid.val+1 .. $].dup;
                _children.length = from.sid.val+1;
            }
            foreach (child; oldChildren)
                child.destroy();
        }

        // and perhaps propagate
        if (!catches() && _parent !is null) {
            _parent.throwCancel(this);
        }
    }
  
    // compare by the SID
    int opCmp(Action r) { return _sid.opCmp(r._sid); }
    bool opEquals(Action r) { return _sid.opEquals(r._sid); }

    // accessors
    SID sid() { return _sid; }
    APGlobalContext gctx() { return _gctx; }
    PASTNode ast() { return _ast; }
    void ast(PASTNode set) { _ast = set; }
    APObject ctx() { return _ctx; }
    APAccessor[] temps() { return _temps; }
    Action[] children() { return _children; }

    bool catches() {
      return _excptHandler !is null;
    }
    APObject getExcptHandler() {
      return _excptHandler;
    }
    void setExcptHandler(APObject excptHandler) {
      _excptHandler = excptHandler;
    }

    /// Current state of the action
    ActionState state;

    /// Whether it's queued
    bool queued;

    /// The serial commit thread has to wait on actions to become 'done'
    Mutex stateMutex;
    /// ditto
    Condition stateCondition;

    private {
        Action _parent;
        SID _sid;
        APGlobalContext _gctx;
        PASTNode _ast;
        APObject _ctx;
        APAccessor[] _temps;

	// If this attribute is non null then this action can
	// catch exceptions, and when it does it runs this code.
	APObject _excptHandler;

        Action[] _children;

        /* children are created under a child sid, so that children which need
         * to be canceled will always be before uncanceled children */
        SID _csid;

        // things to be undone if this is destroyed
        void delegate(Action)[] _undos;

        // things to do if this is committed
        void delegate(Action)[] _commits;
    }
}

class ActionCreationException : Exception {
    this(char[] msg) { super(msg); }
}

/// Global execution context of Plof
class APGlobalContext {
    this(APThreadPool tp) {
        this.tp = tp;
        nul = new APObject();
        initAction = new Action(new SID(0, null), this, null, nul, []);
        nul.setParent(initAction, nul);
        global = new APObject(initAction, nul);
    }

    APThreadPool tp;
    Action initAction;
    APObject nul, global;
}
