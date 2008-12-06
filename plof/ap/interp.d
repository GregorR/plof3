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

import plof.ap.action;
import plof.ap.apobject;
import plof.ap.serial;

import plof.ast.ast;

/// Global execution context of Plof
class APGlobalContext {
    this() {
        initAction = new Action(new SID(0, null), null);
        nul = new APObject();
        nul.setParent(initAction, nul);
        global = new APObject(initAction, nul);
    }

    Action initAction;
    APObject nul, global;
}

/// The interpreter visitor
class APInterpVisitor : PASTVisitor {
    /// Will be visiting within a context, so need the context object
    this(APGlobalContext gctx, Action act, APObject context) {
        _gctx = gctx;
        _act = act;
        _ctx = context;
    }

    private {
        APGlobalContext _gctx;
        Action _act;
        APObject _ctx;
    }



    Object visit(PASTArguments node) { throw new APUnimplementedException("PASTArguments"); }

    Object visit(PASTNull node) { throw new APUnimplementedException("PASTNull"); }

    Object visit(PASTThis node) { throw new APUnimplementedException("PASTThis"); }

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

    Object visit(PASTPrint node) { throw new APUnimplementedException("PASTPrint"); }

    Object visit(PASTCombine node) { throw new APUnimplementedException("PASTCombine"); }

    Object visit(PASTMember node) { throw new APUnimplementedException("PASTMember"); }

    Object visit(PASTCall node) { throw new APUnimplementedException("PASTCall"); }

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

    Object visit(PASTMemberSet node) { throw new APUnimplementedException("PASTMemberSet"); }

    Object visit(PASTArrayIndexSet node) { throw new APUnimplementedException("PASTArrayIndexSet"); }

    Object visit(PASTCmp node) { throw new APUnimplementedException("PASTCmp"); }

    Object visit(PASTIntCmp node) { throw new APUnimplementedException("PASTIntCmp"); }

    Object visit(PASTProc node) { throw new APUnimplementedException("PASTProc"); }

    Object visit(PASTTempSet node) { throw new APUnimplementedException("PASTTempSet"); }

    Object visit(PASTTempGet node) { throw new APUnimplementedException("PASTTempGet"); }

    Object visit(PASTResolve node) { throw new APUnimplementedException("PASTResolve"); }

    Object visit(PASTLoop node) { throw new APUnimplementedException("PASTLoop"); }

    Object visit(PASTArray node) { throw new APUnimplementedException("PASTArray"); }

    Object visit(PASTNativeInteger node) { throw new APUnimplementedException("PASTNativeInteger"); }

    Object visit(PASTRaw node) { throw new APUnimplementedException("PASTRaw"); }
}

class APUnimplementedException : Exception {
    this(char[] msg) { super(msg); }
}
