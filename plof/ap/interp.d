/**
 * Automatically-parallelizing interpreter for the PSL AST
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

module plof.ap.interp;

import tango.io.Stdout;

public import plof.ap.apobject;
import plof.ap.serial;

import plof.ast.ast;

/// The interpreter visitor
class APInterpVisitor : PASTVisitor {
    /// Will be visiting within a context, so need the context object
    this(Action act) {
        _act = act;
    }

    private Action _act;



    Object visit(PASTArguments node) { throw new APUnimplementedException("PASTArguments"); }

    Object visit(PASTNull node) {
        return _act.gctx.nul;
    }

    Object visit(PASTThis node) {
        return _act.ctx;
    }

    Object visit(PASTGlobal node) { throw new APUnimplementedException("PASTGlobal"); }

    Object visit(PASTNew node) { throw new APUnimplementedException("PASTNew"); }

    Object visit(PASTIntWidth node) { throw new APUnimplementedException("PASTIntWidth"); }

    Object visit(PASTVersion node) { throw new APUnimplementedException("PASTVersion"); }

    Object visit(PASTParent node) { throw new APUnimplementedException("PASTParent"); }

    Object visit(PASTReturn node) { throw new APUnimplementedException("PASTReturn"); }

    Object visit(PASTThrow node) { throw new APUnimplementedException("PASTThrow"); }

    Object visit(PASTArrayLength node) { throw new APUnimplementedException("PASTArrayLength"); }

    Object visit(PASTMembers node) { throw new APUnimplementedException("PASTMembers"); }

    Object visit(PASTInteger node) { throw new APUnimplementedException("PASTInteger"); }

    Object visit(PASTByte node) { throw new APUnimplementedException("PASTByte"); }

    Object visit(PASTPrint node) {
        // FIXME: this should not print immediately
        APObject toprint = cast(APObject) node.a1.accept(this);

        // get the data
        ubyte[] raw = toprint.getRaw(_act);

        // and print it
        Stdout(cast(char[]) raw).newline;

        return _act.gctx.nul;
    }

    Object visit(PASTCombine node) { throw new APUnimplementedException("PASTCombine"); }

    Object visit(PASTMember node) {
        // a1.a2
        APObject obj = cast(APObject) node.a1.accept(this);
        APObject name = cast(APObject) node.a2.accept(this);

        // make sure it has a name
        ubyte[] raw = name.getRaw(_act);
        if (raw.length == 0) {
            throw new APInterpFailure("Tried to read the null member.");
        }

        // and get it
        APObject m = obj.getMember(_act, raw);
        if (m is null) {
            return _act.gctx.nul;
        } else {
            return m;
        }
    }

    Object visit(PASTCall node) {
        // a1 = function, a2 = argument
        APObject f = cast(APObject) node.a1.accept(this);
        APObject a = cast(APObject) node.a2.accept(this);

        // make sure it's really a function
        PASTNode fast = f.getAST(_act);
        if (fast is null) {
            throw new APInterpFailure("Called uninitialized thing.");
        }
        PASTProc fproc = cast(PASTProc) fast;
        if (fproc is null) {
            throw new APInterpFailure("Called a non-procedure.");
        }

        /* now make sub-actions for all the steps in the procedure
        Action[] steps;
        steps.length = fproc.stmts.length;
        foreach (i, ast; fproc.stmts) {
            steps ~= new Action(new SID(i, _act.sid), ast);
        } */

        // make the context for this call
        APObject nctx = new APObject(_act, f.getParent(_act));

        // and temps
        APAccessor[] temps;
        temps.length = fproc.temps;
        foreach (i, _; temps) {
            temps[i] = new APAccessor();
        }

        // FIXME: not actually parallel :)
        // and run them
        APObject r;
        foreach (i, ast; fproc.stmts) {
            r = cast(APObject) ast.accept(
                new APInterpVisitor(
                    new Action(new SID(i, _act.sid), _act.gctx, ast, nctx, a, temps)
                )
            );
        }

        return r;
    }

    Object visit(PASTCatch node) { throw new APUnimplementedException("PASTCatch"); }

    Object visit(PASTConcat node) { throw new APUnimplementedException("PASTConcat"); }

    Object visit(PASTWrap node) { throw new APUnimplementedException("PASTWrap"); }

    Object visit(PASTParentSet node) { throw new APUnimplementedException("PASTParentSet"); }

    Object visit(PASTArrayConcat node) { throw new APUnimplementedException("PASTArrayConcat"); }

    Object visit(PASTArrayLengthSet node) { throw new APUnimplementedException("PASTArrayLengthSet"); }

    Object visit(PASTArrayIndex node) { throw new APUnimplementedException("PASTArrayIndex"); }

    Object visit(PASTMul node) { throw new APUnimplementedException("PASTMul"); }

    Object visit(PASTDiv node) { throw new APUnimplementedException("PASTDiv"); }

    Object visit(PASTMod node) { throw new APUnimplementedException("PASTMod"); }

    Object visit(PASTAdd node) { throw new APUnimplementedException("PASTAdd"); }

    Object visit(PASTSub node) { throw new APUnimplementedException("PASTSub"); }

    Object visit(PASTSL node) { throw new APUnimplementedException("PASTSL"); }

    Object visit(PASTSR node) { throw new APUnimplementedException("PASTSR"); }

    Object visit(PASTBitwiseAnd node) { throw new APUnimplementedException("PASTBitwiseAnd"); }

    Object visit(PASTBitwiseNAnd node) { throw new APUnimplementedException("PASTBitwiseNAnd"); }

    Object visit(PASTBitwiseOr node) { throw new APUnimplementedException("PASTBitwiseOr"); }

    Object visit(PASTBitwiseNOr node) { throw new APUnimplementedException("PASTBitwiseNOr"); }

    Object visit(PASTBitwiseXOr node) { throw new APUnimplementedException("PASTBitwiseXOr"); }

    Object visit(PASTBitwiseNXOr node) { throw new APUnimplementedException("PASTBitwiseNXOr"); }

    Object visit(PASTMemberSet node) {
        // set a1.a2 = a3
        APObject trg = cast(APObject) node.a1.accept(this);
        APObject name = cast(APObject) node.a2.accept(this);
        APObject val = cast(APObject) node.a3.accept(this);

        // make sure name actually has a name
        ubyte[] raw = name.getRaw(_act);
        if (raw.length == 0) {
            throw new APInterpFailure("Trying to set the null member.");
        }

        // then set it
        trg.setMember(_act, raw, val);

        return _act.gctx.nul;
    }

    Object visit(PASTArrayIndexSet node) { throw new APUnimplementedException("PASTArrayIndexSet"); }

    Object visit(PASTCmp node) { throw new APUnimplementedException("PASTCmp"); }

    Object visit(PASTIntCmp node) { throw new APUnimplementedException("PASTIntCmp"); }

    Object visit(PASTProc node) {
        // just wrap it in an object
        APObject ret = new APObject(_act, _act.ctx);
        ret.setAST(_act, node);
        return ret;
    }

    Object visit(PASTTempSet node) {
        APObject to = cast(APObject) node.to.accept(this);
        _act.temps[node.tnum].write(_act, to);
        return _act.gctx.nul;
    }

    Object visit(PASTTempGet node) { throw new APUnimplementedException("PASTTempGet"); }

    Object visit(PASTResolve node) { throw new APUnimplementedException("PASTResolve"); }

    Object visit(PASTLoop node) { throw new APUnimplementedException("PASTLoop"); }

    Object visit(PASTArray node) { throw new APUnimplementedException("PASTArray"); }

    Object visit(PASTNativeInteger node) { throw new APUnimplementedException("PASTNativeInteger"); }

    Object visit(PASTRaw node) {
        APObject ret = new APObject(_act, _act.ctx);
        ret.setRaw(_act, node.data.dup);
        return ret;
    }
}

class APUnimplementedException : Exception {
    this(char[] msg) { super(msg); }
}

class APInterpFailure : Exception {
    this(char[] msg) { super(msg); }
}
