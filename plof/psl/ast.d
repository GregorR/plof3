/**
 * PSL AST nodes
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

module plof.psl.ast;

class PASTNode {
    /// If a node uses the heap with anything more complicated than 'new', it can't be reordered
    bool usesHeap() {
        // most don't
        return false;
    }

    /// If a node has effects, it can't be removed as unused
    bool hasEffects() {
        // most don't
        return false;
    }
}


/// Nullary nodes
class PASTNullary : PASTNode {}

/// Get the arguments to this procedure
class PASTArguments : PASTNullary {}

/// Push null
class PASTNull : PASTNullary {}

/// 'this' of this function
class PASTThis : PASTNullary {}

/// Global pointer
class PASTGlobal : PASTNullary {}

/// A new object
class PASTNew : PASTNullary {}

/// Width of native integers
class PASTIntWidth : PASTNullary {}

/// Version constants
class PASTVersion : PASTNullary {}


/// Unary nodes
class PASTUnary : PASTNode {
    this(PASTNode a1) {
        _a1 = a1;
    }

    bool usesHeap() { return hasEffects() || _a1.usesHeap(); }
    bool hasEffects() { return _a1.hasEffects(); }

    PASTNode a1() { return _a1; }

    private PASTNode _a1;
}

/// Parent of an object
class PASTParent : PASTUnary {
    this(PASTNode a1) { super(a1); }
    bool usesHeap() { return true; }
}

/// Return from a function
class PASTReturn : PASTUnary {
    this(PASTNode a1) { super(a1); }
}

/// Throw an exception
class PASTThrow : PASTUnary {
    this(PASTNode a1) { super(a1); }
}

/// Length of an array
class PASTArrayLength : PASTUnary {
    this(PASTNode a1) { super(a1); }
}

/// Members of an object
class PASTMembers : PASTUnary {
    this(PASTNode a1) { super(a1); }
}

/// Non-native integer
class PASTInteger : PASTUnary {
    this(PASTNode a1) { super(a1); }
}

/// Integer-to-byte
class PASTByte : PASTUnary {
    this(PASTNode a1) { super(a1); }
}

/// Debugging print
class PASTPrint : PASTUnary {
    this(PASTNode a1) { super(a1); }
}


/// Binary nodes
class PASTBinary : PASTUnary {
    this(PASTNode a1, PASTNode a2) {
        super(a1);
        _a2 = a2;
    }

    bool usesHeap() { return super.usesHeap() || _a2.usesHeap(); }
    bool hasEffects() { return super.hasEffects() || _a2.hasEffects(); }

    PASTNode a2() { return _a2; }

    private PASTNode _a2;
}

/// Combine two objects
class PASTCombine : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
}

/// Get a member of an object
class PASTMember : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
    bool usesHeap() { return true; }
}

/// Call a function
class PASTCall : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
    bool hasEffects() { return true; }
}

/// Catch exceptions
class PASTCatch : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
    bool hasEffects() { return true; }
}

/// Concatenation
class PASTConcat : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
}

/// Wrap
class PASTWrap : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
}

/// Array concatenation
class PASTArrayConcat : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
}

/// Set the length of an array
class PASTArrayLengthSet : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
}

/// Get an element from an array
class PASTArrayIndex : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
}

/// Math nodes
class PASTMul : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
}
class PASTDiv : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
}
class PASTMod : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
}
class PASTAdd : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
}
class PASTSub : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
}
class PASTSL : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
}
class PASTSR : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
}
class PASTBitwiseAnd : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
}
class PASTBitwiseNAnd : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
}
class PASTBitwiseOr : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
}
class PASTBitwiseNOr : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
}
class PASTBitwiseXOr : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
}
class PASTBitwiseNXOr : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
}


/// Trinary nodes
class PASTTrinary : PASTBinary {
    this(PASTNode a1, PASTNode a2, PASTNode a3) {
        super(a1, a2);
        _a3 = a3;
    }

    bool usesHeap() { return super.usesHeap() || _a3.usesHeap(); }
    bool hasEffects() { return super.hasEffects() || _a3.hasEffects(); }

    PASTNode a3() { return _a3; }

    private PASTNode _a3;
}

/// Set a member
class PASTMemberSet : PASTTrinary {
    this(PASTNode a1, PASTNode a2, PASTNode a3) { super(a1, a2, a3); }

    bool usesHeap() { return true; }
    bool hasEffects() { return true; }
}

/// Set an index of an array
class PASTArrayIndexSet : PASTTrinary {
    this(PASTNode a1, PASTNode a2, PASTNode a3) { super(a1, a2, a3); }

    bool usesHeap() { return true; }
    bool hasEffects() { return true; }
}


/// Quaternary nodes
class PASTQuaternary : PASTTrinary {
    this(PASTNode a1, PASTNode a2, PASTNode a3, PASTNode a4) {
        super(a1, a2, a3);
        _a4 = a4;
    }

    bool usesHeap() { return super.usesHeap() || _a4.usesHeap(); }
    bool hasEffects() { return super.hasEffects() || _a4.hasEffects(); }

    PASTNode a4() { return _a4; }

    private PASTNode _a4;
}

/// Compare objects
class PASTCmp : PASTQuaternary {
    this(PASTNode a1, PASTNode a2, PASTNode a3, PASTNode a4) {
        super(a1, a2, a3, a4);
    }
    bool hasEffects() { return true; }
}

/// Integer comparisons
class PASTIntCmp : PASTQuaternary {
    this(PASTNode a1, PASTNode a2, PASTNode a3, PASTNode a4, ubyte cmd) {
        super(a1, a2, a3, a4);
        _cmd = cmd;
    }
    bool hasEffects() { return true; }

    ubyte cmd() { return _cmd; }

    private ubyte _cmd;
}


/// A procedure
class PASTProc : PASTNode {
    this(PASTNode[] stmts, uint temps, char[] dfile, int dline, int dcol) {
        _stmts = stmts;
        _temps = temps;
        _dfile = dfile.dup;
        _dline = dline;
        _dcol = dcol;
    }

    PASTNode[] stmts() { return _stmts; }

    private {
        /// The statements in this proc
        PASTNode[] _stmts;

        /// How many temps this needs
        uint _temps;

        /// Debugging info
        char[] _dfile;
        int _dline;
        int _dcol;
    }
}

/// Set a temporary
class PASTTempSet : PASTNode {
    this(uint tnum, PASTNode to) {
        _tnum = tnum;
        _to = to;
    }

    bool hasEffects() { return true; }

    private {
        /// Temporary number and value
        uint _tnum;
        PASTNode _to;
    }
}

/// Get a temporary
class PASTTempGet : PASTNode {
    this(uint tnum) {
        _tnum = tnum;
    }

    /// Temporary number
    private uint _tnum;
}

/// Resolve a name (into two temporaries)
class PASTResolve : PASTNode {
    this(PASTNode obj, PASTNode name, uint t1, uint t2) {
        _obj = obj;
        _name = name;
        _t1 = t1;
        _t2 = t2;
    }

    bool usesHeap() { return true; }
    bool hasEffects() { return _obj.hasEffects() || _name.hasEffects(); }

    private {
        /// The object and name to resolve
        PASTNode _obj, _name;

        /// The temporaries to put it in
        uint _t1, _t2;
    }
}

/// Loop a list of statments
class PASTLoop : PASTNode {
    this(PASTNode[] stmts) {
        _stmts = stmts;
    }

    // usesHeap and hasEffects are complicated because all sub-statements must be checked
    bool usesHeap() {
        foreach (stmt; stmts) {
            if (stmt.usesHeap()) return true;
        }
        return false;
    }
    bool hasEffects() {
        foreach (stmt; stmts) {
            if (stmt.hasEffects()) return true;
        }
        return false;
    }

    PASTNode[] stmts() { return _stmts; }

    private PASTNode[] _stmts;
}

/// Creation of an array
class PASTArray : PASTNode {
    this(PASTNode[] elems) {
        _elems = elems;
    }

    // usesHeap and hasEffects are complicated because all sub-elements must be checked
    bool usesHeap() {
        foreach (elem; elems) {
            if (elem.usesHeap()) return true;
        }
        return false;
    }
    bool hasEffects() {
        foreach (elem; elems) {
            if (elem.hasEffects()) return true;
        }
        return false;
    }

    PASTNode[] elems() { return _elems; }

    private PASTNode[] _elems;
}

/// Native integer
class PASTNativeInteger : PASTNode {
    this(int value) {
        _value = value;
    }

    int value() { return _value; }

    private int _value;
}

/// Raw data
class PASTRaw : PASTNode {
    this(ubyte[] data) {
        _data = data.dup;
    }

    ubyte[] data() { return _data; }

    private ubyte[] _data;
}
