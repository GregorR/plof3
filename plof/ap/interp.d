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


    /// Generic function for calling a function with an argument
    APObject call(APObject func, APObject arg) {
        // make sure it's really a function
        PASTNode fast = func.getAST(_act);
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
        APObject nctx = new APObject(_act, func.getParent(_act));

        // and temps
        APAccessor[] temps;
        temps.length = fproc.temps;
        foreach (i, _; temps) {
            temps[i] = new APAccessor();
        }

        // FIXME: not actually parallel :)
        // and run them
        foreach (i, ast; fproc.stmts) {
            ast.accept(
                new APInterpVisitor(
                    new Action(new SID(i, _act.sid), _act.gctx, ast, nctx, arg, temps)
                )
            );
        }

        return nctx;
    }



    Object visit(PASTArguments node) {
        return _act.arg;
    }

    Object visit(PASTNull node) {
        return _act.gctx.nul;
    }

    Object visit(PASTThis node) {
        return _act.ctx;
    }

    Object visit(PASTGlobal node) {
        return _act.gctx.global;
    }

    Object visit(PASTNew node) {
        return new APObject(_act, _act.ctx);
    }

    Object visit(PASTIntWidth node) { throw new APUnimplementedException("PASTIntWidth"); }

    Object visit(PASTVersion node) { throw new APUnimplementedException("PASTVersion"); }

    Object visit(PASTParent node) { throw new APUnimplementedException("PASTParent"); }

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
        if (raw.length == 0) {
            Stdout(cast(void*) toprint).newline;
        } else {
            Stdout(cast(char[]) raw).newline;
        }

        return _act.gctx.nul;
    }

    Object visit(PASTReturnGet node) {
        // get the return from a context
        APObject ctx = cast(APObject) node.a1.accept(this);

        APObject ret = ctx.getMember(_act, cast(ubyte[]) "\x1breturn");
        if (ret is null)
            ret = _act.gctx.nul;
        return ret;
    }

    Object visit(PASTCombine node) {
        // a1:a2
        APObject l = cast(APObject) node.a1.accept(this);
        APObject r = cast(APObject) node.a2.accept(this);
        APObject ret = new APObject(_act, r.getParent(_act));

        // get all the members of both
        ubyte[][] lmembers = l.getMembers(_act);
        ubyte[][] rmembers = r.getMembers(_act);

        // and combine them
        foreach (lmember; lmembers) {
            ret.setMember(_act, lmember, l.getMember(_act, lmember));
        }
        foreach (rmember; rmembers) {
            ret.setMember(_act, rmember, r.getMember(_act, rmember));
        }

        // now combine any other data
        ubyte[] lraw = l.getRaw(_act);
        ubyte[] rraw = r.getRaw(_act);
        if (lraw.length != 0 || rraw.length != 0) {
            // one of them had raw data, so make this a raw
            ret.setRaw(_act, lraw ~ rraw);

        } else {
            // neither had raw, check for array
            uint llen = l.getArrayLength(_act);
            uint rlen = r.getArrayLength(_act);
            if (llen != 0 || rlen != 0) {
                // had array data, so make an array
                ret.setArrayLength(_act, llen + rlen);

                // copy in the elements
                for (uint i = 0; i < llen; i++) {
                    ret.setArrayElement(_act, i, l.getArrayElement(_act, i));
                }
                for (uint i = 0; i < rlen; i++) {
                    ret.setArrayElement(_act, llen+i, r.getArrayElement(_act, i));
                }

            }

        }

        return ret;
    }

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

        return call(f, a);
    }

    Object visit(PASTCatch node) { throw new APUnimplementedException("PASTCatch"); }

    Object visit(PASTConcat node) { throw new APUnimplementedException("PASTConcat"); }

    Object visit(PASTWrap node) { throw new APUnimplementedException("PASTWrap"); }

    Object visit(PASTParentSet node) {
        // a1.parent = a2
        APObject trg = cast(APObject) node.a1.accept(this);
        APObject obj = cast(APObject) node.a2.accept(this);

        trg.setParent(_act, obj);

        return _act.gctx.nul;
    }

    Object visit(PASTArrayConcat node) { throw new APUnimplementedException("PASTArrayConcat"); }

    Object visit(PASTArrayLengthSet node) { throw new APUnimplementedException("PASTArrayLengthSet"); }

    Object visit(PASTArrayIndex node) {
        // a1[a2]
        APObject obj = cast(APObject) node.a1.accept(this);
        APObject index = cast(APObject) node.a2.accept(this);

        return obj.getArrayElement(_act, index.getInteger(_act));
    }

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

    Object visit(PASTReturnSet node) {
        // set the return of this context
        APObject ctx = cast(APObject) node.a1.accept(this);
        APObject val = cast(APObject) node.a2.accept(this);

        ctx.setMember(_act, cast(ubyte[]) "\x1breturn", val);

        return _act.gctx.nul;
    }

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

    Object visit(PASTCmp node) {
        // if (a2 is a3) a4(a1) else a5(a1)
        APObject arg = cast(APObject) node.a1.accept(this);
        APObject l = cast(APObject) node.a2.accept(this);
        APObject r = cast(APObject) node.a3.accept(this);
        APObject ift = cast(APObject) node.a4.accept(this);
        APObject iff = cast(APObject) node.a5.accept(this);
        APObject toc;

        if (l is r) {
            toc = ift;
        } else {
            toc = iff;
        }

        // call the appropriate one
        return call(toc, arg);
    }

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

    Object visit(PASTTempGet node) {
        APObject ret = _act.temps[node.tnum].read(_act);
        if (ret is null) {
            return _act.gctx.nul;
        } else {
            return ret;
        }
    }

    Object visit(PASTResolve node) {
        // resolve a1.[a2]
        APObject obj = cast(APObject) node.obj.accept(this);
        APObject onames = cast(APObject) node.name.accept(this);
        ubyte[][] names;
        APObject[] nameobjs;
        
        // get out the names
        if (onames.isArray(_act)) {
            // OK, get them from the elements
            for (uint i = 0; i < onames.getArrayLength(_act); i++) {
                APObject robj = onames.getArrayElement(_act, i);
                ubyte[] raw;
                if (robj !is null) raw = robj.getRaw(_act);
                if (raw.length != 0) {
                    names ~= raw;
                    nameobjs ~= robj;
                }
            }

        } else {
            // just get the one
            ubyte[] raw = onames.getRaw(_act);
            if (raw.length != 0) {
                names ~= raw;
                nameobjs ~= onames;
            }

        }

        // make sure there are at least a few elements
        if (names.length == 0) {
            // well this is just screwy!
            _act.temps[node.t1].write(_act, _act.gctx.nul);
            _act.temps[node.t2].write(_act, _act.gctx.nul);
            return _act.gctx.nul;
        }

        // OK, find an object
        while (obj !is null && obj !is _act.gctx.nul) {
            foreach (namei, name; names) {
                APObject ret = obj.getMember(_act, name);
                if (ret !is null && ret !is _act.gctx.nul) {
                    // found it!
                    _act.temps[node.t1].write(_act, obj);
                    _act.temps[node.t2].write(_act, nameobjs[namei]);
                    return _act.gctx.nul;
                }

            }

            obj = obj.getParent(_act);
        }

        // didn't find it
        _act.temps[node.t1].write(_act, _act.gctx.nul);
        _act.temps[node.t2].write(_act, _act.gctx.nul);
        return _act.gctx.nul;
    }

    Object visit(PASTLoop node) { throw new APUnimplementedException("PASTLoop"); }

    Object visit(PASTArray node) {
        APObject ret = new APObject(_act, _act.ctx);

        // create the elements
        ret.setArrayLength(_act, node.elems.length);
        foreach (i, elem; node.elems) {
            ret.setArrayElement(_act, i, cast(APObject) elem.accept(this));
        }

        return ret;
    }

    Object visit(PASTNativeInteger node) {
        APObject ret = new APObject(_act, _act.ctx);

        // some weird casting will get the data right
        ptrdiff_t val = node.value;
        ret.setRaw(_act, (cast(ubyte*) &val)[0..ptrdiff_t.sizeof].dup);

        return ret;
    }

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
