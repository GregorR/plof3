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

module plof.ast.ast;

import tango.text.Util;
import tango.text.convert.Integer;
alias tango.text.convert.Integer.toString intToString;

import tango.io.Stdout;

// necessary helper function to get a short classname
private char[] classShortName(char[] classn)
{
    uint i = locatePattern(classn, "PAST");
    if (i == classn.length) {
        return classn;
    } else {
        return classn[i+4 .. $];
    }
}

/// Standard visitor stuff
private char[] acceptors;
template Accept() {
    Object accept(PASTVisitor v) { return v.visit(this); }
}


/// The base for all AST nodes
class PASTNode {
    // can't actually create PASTNodes
    private this() {}

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

    /// Convert to an XML-like AST tree
    char[] toXML() { return "<" ~ classShortName(this.classinfo.name) ~ " BAD=\"YES\"/>"; }

    /// Only those with mixin Accept can be created
    abstract Object accept(PASTVisitor v);
}


/// Nullary nodes
class PASTNullary : PASTNode {
    char[] toXML() {
        return "<" ~ classShortName(this.classinfo.name) ~ "/>";
    }
}

/// Get the arguments to this procedure
class PASTArguments : PASTNullary { mixin Accept; }

/// Push null
class PASTNull : PASTNullary { mixin Accept; }

/// 'this' of this function
class PASTThis : PASTNullary { mixin Accept; }

/// Global pointer
class PASTGlobal : PASTNullary { mixin Accept; }

/// A new object
class PASTNew : PASTNullary { mixin Accept; }

/// Width of native integers
class PASTIntWidth : PASTNullary { mixin Accept; }

/// Version constants
class PASTVersion : PASTNullary { mixin Accept; }


/// Unary nodes
class PASTUnary : PASTNode {
    this(PASTNode a1) {
        _a1 = a1;
    }

    bool usesHeap() { return hasEffects() || _a1.usesHeap(); }
    bool hasEffects() { return _a1.hasEffects(); }

    PASTNode a1() { return _a1; }

    char[] toXML() {
        return "<" ~ classShortName(this.classinfo.name) ~ ">" ~ _a1.toXML() ~
            "</" ~ classShortName(this.classinfo.name) ~ ">";
    }

    private PASTNode _a1;
}

/// Parent of an object
class PASTParent : PASTUnary {
    this(PASTNode a1) { super(a1); }
    bool usesHeap() { return true; }
    mixin Accept;
}

/// Return from a function
class PASTReturn : PASTUnary {
    this(PASTNode a1) { super(a1); }
    mixin Accept;
}

/// Throw an exception
class PASTThrow : PASTUnary {
    this(PASTNode a1) { super(a1); }
    mixin Accept;
}

/// Length of an array
class PASTArrayLength : PASTUnary {
    this(PASTNode a1) { super(a1); }
    mixin Accept;
}

/// Members of an object
class PASTMembers : PASTUnary {
    this(PASTNode a1) { super(a1); }
    mixin Accept;
}

/// Non-native integer
class PASTInteger : PASTUnary {
    this(PASTNode a1) { super(a1); }
    mixin Accept;
}

/// Integer-to-byte
class PASTByte : PASTUnary {
    this(PASTNode a1) { super(a1); }
    mixin Accept;
}

/// Debugging print
class PASTPrint : PASTUnary {
    this(PASTNode a1) { super(a1); }
    bool hasEffects() { return true; }
    mixin Accept;
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

    char[] toXML() {
        return "<" ~ classShortName(this.classinfo.name) ~ ">" ~ _a1.toXML() ~
            _a2.toXML() ~ "</" ~ classShortName(this.classinfo.name) ~ ">";
    }

    private PASTNode _a2;
}

/// Combine two objects
class PASTCombine : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
    mixin Accept;
}

/// Get a member of an object
class PASTMember : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
    bool usesHeap() { return true; }
    mixin Accept;
}

/// Call a function
class PASTCall : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
    bool hasEffects() { return true; }
    mixin Accept;
}

/// Catch exceptions
class PASTCatch : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
    bool hasEffects() { return true; }
    mixin Accept;
}

/// Concatenation
class PASTConcat : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
    mixin Accept;
}

/// Wrap
class PASTWrap : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
    mixin Accept;
}

/// Set the parent of an object
class PASTParentSet : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
    mixin Accept;
}

/// Array concatenation
class PASTArrayConcat : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
    mixin Accept;
}

/// Set the length of an array
class PASTArrayLengthSet : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
    mixin Accept;
}

/// Get an element from an array
class PASTArrayIndex : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
    mixin Accept;
}

/// Math nodes
class PASTMul : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
    mixin Accept;
}
class PASTDiv : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
    mixin Accept;
}
class PASTMod : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
    mixin Accept;
}
class PASTAdd : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
    mixin Accept;
}
class PASTSub : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
    mixin Accept;
}
class PASTSL : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
    mixin Accept;
}
class PASTSR : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
    mixin Accept;
}
class PASTBitwiseAnd : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
    mixin Accept;
}
class PASTBitwiseNAnd : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
    mixin Accept;
}
class PASTBitwiseOr : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
    mixin Accept;
}
class PASTBitwiseNOr : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
    mixin Accept;
}
class PASTBitwiseXOr : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
    mixin Accept;
}
class PASTBitwiseNXOr : PASTBinary {
    this(PASTNode a1, PASTNode a2) { super(a1, a2); }
    mixin Accept;
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

    char[] toXML() {
        return "<" ~ classShortName(this.classinfo.name) ~ ">" ~ _a1.toXML() ~
            _a2.toXML() ~ _a3.toXML() ~ "</" ~
            classShortName(this.classinfo.name) ~ ">";
    }

    private PASTNode _a3;
}

/// Set a member
class PASTMemberSet : PASTTrinary {
    this(PASTNode a1, PASTNode a2, PASTNode a3) { super(a1, a2, a3); }

    bool usesHeap() { return true; }
    bool hasEffects() { return true; }

    mixin Accept;
}

/// Set an index of an array
class PASTArrayIndexSet : PASTTrinary {
    this(PASTNode a1, PASTNode a2, PASTNode a3) { super(a1, a2, a3); }

    bool usesHeap() { return true; }
    bool hasEffects() { return true; }

    mixin Accept;
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

    char[] toXML() {
        return "<" ~ classShortName(this.classinfo.name) ~ ">" ~ _a1.toXML() ~
            _a2.toXML() ~ _a3.toXML() ~ _a4.toXML() ~ "</" ~
            classShortName(this.classinfo.name) ~ ">";
    }

    private PASTNode _a4;
}

/// Compare objects
class PASTCmp : PASTQuaternary {
    this(PASTNode a1, PASTNode a2, PASTNode a3, PASTNode a4) {
        super(a1, a2, a3, a4);
    }
    bool hasEffects() { return true; }
    mixin Accept;
}

/// Integer comparisons
class PASTIntCmp : PASTQuaternary {
    this(PASTNode a1, PASTNode a2, PASTNode a3, PASTNode a4, ubyte cmd) {
        super(a1, a2, a3, a4);
        _cmd = cmd;
    }
    bool hasEffects() { return true; }

    ubyte cmd() { return _cmd; }

    mixin Accept;

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
    uint temps() { return _temps; }

    char[] toXML() {
        char[] ret = "<Proc temps=\"" ~ intToString(_temps) ~ "\"";

        // only output debug info if we have it
        if (_dfile.length) {
            ret ~= " file=\"" ~ _dfile ~ "\" line=\"" ~ intToString(_dline) ~
                "\" col=\"" ~ intToString(_dcol) ~ "\"";
        }

        ret ~= ">";

        // now go through each of the elements
        foreach (stmt; stmts) {
            ret ~= stmt.toXML();
        }

        ret ~= "</Proc>";

        return ret;
    }

    mixin Accept;

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

    uint tnum() { return _tnum; }
    PASTNode to() { return _to; }

    char[] toXML() {
        return "<TempSet temp=\"" ~ intToString(_tnum) ~ "\">" ~ _to.toXML() ~ "</TempSet>";
    }

    mixin Accept;

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

    char[] toXML() {
        return "<TempGet temp=\"" ~ intToString(_tnum) ~ "\"/>";
    }

    mixin Accept;

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

    // accessors
    PASTNode obj() { return _obj; }
    PASTNode name() { return _name; }
    uint t1() { return _t1; }
    uint t2() { return _t2; }

    char[] toXML() {
        return "<Resolve t1=\"" ~ intToString(_t1) ~ "\" t2=\"" ~
            intToString(_t2) ~ "\">" ~ _obj.toXML() ~ _name.toXML() ~
            "</Resolve>";
    }

    mixin Accept;

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
        foreach (stmt; _stmts) {
            if (stmt.usesHeap()) return true;
        }
        return false;
    }
    bool hasEffects() {
        foreach (stmt; _stmts) {
            if (stmt.hasEffects()) return true;
        }
        return false;
    }

    PASTNode[] stmts() { return _stmts; }

    char[] toXML() {
        char[] ret = "<Loop>";

        // now go through each of the elements
        foreach (stmt; _stmts) {
            ret ~= stmt.toXML();
        }

        ret ~= "</Loop>";

        return ret;
    }

    mixin Accept;

    private PASTNode[] _stmts;
}

/// Creation of an array
class PASTArray : PASTNode {
    this(PASTNode[] elems) {
        _elems = elems;
    }

    bool usesHeap() { return true; }
    bool hasEffects() { return true; }

    PASTNode[] elems() { return _elems; }

    char[] toXML() {
        char[] ret = "<Array>";

        // now go through each of the elements
        foreach (elem; _elems) {
            ret ~= elem.toXML();
        }

        ret ~= "</Array>";

        return ret;
    }

    mixin Accept;

    private PASTNode[] _elems;
}

/// Native integer
class PASTNativeInteger : PASTNode {
    this(int value) {
        _value = value;
    }

    int value() { return _value; }

    char[] toXML() {
        return "<NativeInteger value=\"" ~ intToString(_value) ~ "\"/>";
    }

    mixin Accept;

    private int _value;
}

/// Raw data
class PASTRaw : PASTNode {
    this(ubyte[] data) {
        _data = data.dup;
    }

    ubyte[] data() { return _data; }

    char[] toXML() {
        char[] ret = "<Raw data=\"";
        
        // Just output each element as an int
        foreach (datum; _data) {
            if (datum >= 32 && datum <= 126 &&
                datum != '&' && datum != ';' && datum != '"') {
                ret ~= cast(char) datum;
            } else {
                ret ~= "&#" ~ intToString(datum) ~ ";";
            }
        }

        ret ~= "\"/>";

        return ret;
    }

    mixin Accept;

    private ubyte[] _data;
}



/// Visitor for PASTNodes
interface PASTVisitor {
    Object visit(PASTArguments);
    Object visit(PASTNull);
    Object visit(PASTThis);
    Object visit(PASTGlobal);
    Object visit(PASTNew);
    Object visit(PASTIntWidth);
    Object visit(PASTVersion);
    Object visit(PASTParent);
    Object visit(PASTReturn);
    Object visit(PASTThrow);
    Object visit(PASTArrayLength);
    Object visit(PASTMembers);
    Object visit(PASTInteger);
    Object visit(PASTByte);
    Object visit(PASTPrint);
    Object visit(PASTCombine);
    Object visit(PASTMember);
    Object visit(PASTCall);
    Object visit(PASTCatch);
    Object visit(PASTConcat);
    Object visit(PASTWrap);
    Object visit(PASTParentSet);
    Object visit(PASTArrayConcat);
    Object visit(PASTArrayLengthSet);
    Object visit(PASTArrayIndex);
    Object visit(PASTMul);
    Object visit(PASTDiv);
    Object visit(PASTMod);
    Object visit(PASTAdd);
    Object visit(PASTSub);
    Object visit(PASTSL);
    Object visit(PASTSR);
    Object visit(PASTBitwiseAnd);
    Object visit(PASTBitwiseNAnd);
    Object visit(PASTBitwiseOr);
    Object visit(PASTBitwiseNOr);
    Object visit(PASTBitwiseXOr);
    Object visit(PASTBitwiseNXOr);
    Object visit(PASTMemberSet);
    Object visit(PASTArrayIndexSet);
    Object visit(PASTCmp);
    Object visit(PASTIntCmp);
    Object visit(PASTProc);
    Object visit(PASTTempSet);
    Object visit(PASTTempGet);
    Object visit(PASTResolve);
    Object visit(PASTLoop);
    Object visit(PASTArray);
    Object visit(PASTNativeInteger);
    Object visit(PASTRaw);
}
