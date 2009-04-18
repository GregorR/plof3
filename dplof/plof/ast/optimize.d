/**
 * Extremely-simple optimizer for PSL ASTs
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

module plof.ast.optimize;

import tango.io.Stdout;

import plof.ast.ast;

/// Optimize an AST
PASTNode optimize(PASTNode x) {
    return cast(PASTNode) x.accept(new PASTOptimizer());
}

class PASTOptimizer : PASTVisitor {
    Object visit(PASTArguments x) {
        return x;
    }
    Object visit(PASTNull x) {
        return x;
    }
    Object visit(PASTThis x) {
        return x;
    }
    Object visit(PASTGlobal x) {
        return x;
    }
    Object visit(PASTNew x) {
        return x;
    }
    Object visit(PASTIntWidth x) {
        return x;
    }
    Object visit(PASTVersion x) {
        return x;
    }
    Object visit(PASTParent x) {
        return new PASTParent(cast(PASTNode) x.a1.accept(this));
    }
    Object visit(PASTThrow x) {
        return new PASTThrow(cast(PASTNode) x.a1.accept(this));
    }
    Object visit(PASTArrayLength x) {
        return new PASTArrayLength(cast(PASTNode) x.a1.accept(this));
    }
    Object visit(PASTMembers x) {
        return new PASTMembers(cast(PASTNode) x.a1.accept(this));
    }
    Object visit(PASTRawLength x) {
        return new PASTRawLength(cast(PASTNode) x.a1.accept(this));
    }
    Object visit(PASTSlice x) {
        return new PASTSlice(cast(PASTNode) x.a1.accept(this), cast(PASTNode) x.a2.accept(this), cast(PASTNode) x.a3.accept(this));
    }
    Object visit(PASTRawCmp x) {
        return new PASTRawCmp(cast(PASTNode) x.a1.accept(this), cast(PASTNode) x.a2.accept(this));
    }
    Object visit(PASTInteger x) {
        return new PASTInteger(cast(PASTNode) x.a1.accept(this));
    }
    Object visit(PASTByte x) {
        return new PASTByte(cast(PASTNode) x.a1.accept(this));
    }
    Object visit(PASTPrint x) {
        return new PASTPrint(cast(PASTNode) x.a1.accept(this));
    }
    Object visit(PASTCombine x) {
        return new PASTCombine(cast(PASTNode) x.a1.accept(this), cast(PASTNode) x.a2.accept(this));
    }
    Object visit(PASTMember x) {
        return new PASTMember(cast(PASTNode) x.a1.accept(this), cast(PASTNode) x.a2.accept(this));
    }
    Object visit(PASTCall x) {
        return new PASTCall(cast(PASTNode) x.a1.accept(this), cast(PASTNode) x.a2.accept(this));
    }
    Object visit(PASTCatch x) {
        return new PASTCatch(cast(PASTNode) x.a1.accept(this), cast(PASTNode) x.a2.accept(this), cast(PASTNode) x.a3.accept(this));
    }

    Object visit(PASTConcat x) {
        // we can optimize proc-proc concats and raw-raw concats
        PASTNode l = cast(PASTNode) x.a1.accept(this);
        PASTNode r = cast(PASTNode) x.a2.accept(this);
        Stdout("Concat of ")(l.toXML())(" and ")(r.toXML()).newline;
        PASTProc pl, pr;
        PASTRaw rl, rr;
        if ((pl = cast(PASTProc) l) !is null) {
            if ((pr = cast(PASTProc) r) !is null) {
                int maxtemps = pl.temps;
                if (pr.temps > maxtemps) maxtemps = pr.temps;
                return new PASTProc(pl.stmts ~ pr.stmts, maxtemps, pl.dfile, pl.dline, pl.dcol);
            } else {
                return new PASTConcat(l, r);
            }

        } else if ((rl = cast(PASTRaw) l) !is null) {
            if ((rr = cast(PASTRaw) r) !is null) {
                return new PASTRaw(rl.data ~ rr.data);
            } else {
                return new PASTConcat(l, r);
            }

        } else {
            return new PASTConcat(l, r);
        }
    }

    Object visit(PASTWrap x) {
        return new PASTWrap(cast(PASTNode) x.a1.accept(this), cast(PASTNode) x.a2.accept(this));
    }
    Object visit(PASTParentSet x) {
        return new PASTParentSet(cast(PASTNode) x.a1.accept(this), cast(PASTNode) x.a2.accept(this));
    }
    Object visit(PASTArrayConcat x) {
        return new PASTArrayConcat(cast(PASTNode) x.a1.accept(this), cast(PASTNode) x.a2.accept(this));
    }
    Object visit(PASTArrayLengthSet x) {
        return new PASTArrayLengthSet(cast(PASTNode) x.a1.accept(this), cast(PASTNode) x.a2.accept(this));
    }
    Object visit(PASTArrayIndex x) {
        return new PASTArrayIndex(cast(PASTNode) x.a1.accept(this), cast(PASTNode) x.a2.accept(this));
    }
    Object visit(PASTMul x) {
        return new PASTMul(cast(PASTNode) x.a1.accept(this), cast(PASTNode) x.a2.accept(this));
    }
    Object visit(PASTDiv x) {
        return new PASTDiv(cast(PASTNode) x.a1.accept(this), cast(PASTNode) x.a2.accept(this));
    }
    Object visit(PASTMod x) {
        return new PASTMod(cast(PASTNode) x.a1.accept(this), cast(PASTNode) x.a2.accept(this));
    }
    Object visit(PASTAdd x) {
        return new PASTAdd(cast(PASTNode) x.a1.accept(this), cast(PASTNode) x.a2.accept(this));
    }
    Object visit(PASTSub x) {
        return new PASTSub(cast(PASTNode) x.a1.accept(this), cast(PASTNode) x.a2.accept(this));
    }
    Object visit(PASTSL x) {
        return new PASTSL(cast(PASTNode) x.a1.accept(this), cast(PASTNode) x.a2.accept(this));
    }
    Object visit(PASTSR x) {
        return new PASTSR(cast(PASTNode) x.a1.accept(this), cast(PASTNode) x.a2.accept(this));
    }
    Object visit(PASTBitwiseAnd x) {
        return new PASTBitwiseAnd(cast(PASTNode) x.a1.accept(this), cast(PASTNode) x.a2.accept(this));
    }
    Object visit(PASTBitwiseNAnd x) {
        return new PASTBitwiseNAnd(cast(PASTNode) x.a1.accept(this), cast(PASTNode) x.a2.accept(this));
    }
    Object visit(PASTBitwiseOr x) {
        return new PASTBitwiseOr(cast(PASTNode) x.a1.accept(this), cast(PASTNode) x.a2.accept(this));
    }
    Object visit(PASTBitwiseNOr x) {
        return new PASTBitwiseNOr(cast(PASTNode) x.a1.accept(this), cast(PASTNode) x.a2.accept(this));
    }
    Object visit(PASTBitwiseXOr x) {
        return new PASTBitwiseXOr(cast(PASTNode) x.a1.accept(this), cast(PASTNode) x.a2.accept(this));
    }
    Object visit(PASTBitwiseNXOr x) {
        return new PASTBitwiseNXOr(cast(PASTNode) x.a1.accept(this), cast(PASTNode) x.a2.accept(this));
    }
    Object visit(PASTMemberSet x) {
        return new PASTMemberSet(cast(PASTNode) x.a1.accept(this), cast(PASTNode) x.a2.accept(this), cast(PASTNode) x.a3.accept(this));
    }
    Object visit(PASTArrayIndexSet x) {
        return new PASTArrayIndexSet(cast(PASTNode) x.a1.accept(this), cast(PASTNode) x.a2.accept(this), cast(PASTNode) x.a3.accept(this));
    }
    Object visit(PASTCmp x) {
        return new PASTCmp(cast(PASTNode) x.a1.accept(this), cast(PASTNode) x.a2.accept(this));
    }
    Object visit(PASTIntCmp x) {
        return new PASTIntCmp(cast(PASTNode) x.a1.accept(this), cast(PASTNode) x.a2.accept(this), x.cmd);
    }
    Object visit(PASTProc x) {
        PASTNode[] newstmts;
        newstmts.length = x.stmts.length;
        foreach (i, stmt; x.stmts) {
            newstmts[i] = cast(PASTNode) stmt.accept(this);
        }
        return new PASTProc(newstmts, x.temps, x.dfile, x.dline, x.dcol);
    }
    Object visit(PASTTempSet x) {
        return new PASTTempSet(x.tnum, cast(PASTNode) x.to.accept(this));
    }
    Object visit(PASTTempGet x) {
        return x;
    }
    Object visit(PASTResolve x) {
        return new PASTResolve(cast(PASTNode) x.obj.accept(this), cast(PASTNode) x.name.accept(this), x.t1, x.t2);
    }
    Object visit(PASTWhile x) {
        return new PASTWhile(
                cast(PASTNode) x.a1.accept(this),
                cast(PASTNode) x.a2.accept(this),
                cast(PASTNode) x.a3.accept(this));
    }
    Object visit(PASTIf x) {
        return new PASTIf(
                cast(PASTNode) x.a1.accept(this),
                cast(PASTNode) x.a2.accept(this),
                cast(PASTNode) x.a3.accept(this),
                cast(PASTNode) x.a4.accept(this));
    }
    Object visit(PASTArray x) {
        PASTNode[] newelems;
        newelems.length = x.elems.length;
        foreach (i, elem; x.elems) {
            newelems[i] = cast(PASTNode) elem.accept(this);
        }
        return new PASTArray(newelems);
    }
    Object visit(PASTNativeInteger x) {
        return x;
    }
    Object visit(PASTRaw x) {
        return x;
    }
}
