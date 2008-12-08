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

import plof.psl.psl;

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
        nctx.setMember(_act, cast(ubyte[]) "\x1bargument", arg);

        // and temps
        APAccessor[] temps;
        temps.length = fproc.temps;
        foreach (i, _; temps) {
            temps[i] = new APAccessor();
        }

        // and put them in a queue
        Action[] toEnqueue;
        toEnqueue.length = fproc.stmts.length;
        foreach (i, ast; fproc.stmts) {
            toEnqueue[i] = _act.createChild(ast, nctx, temps);
        }

        return runsub(toEnqueue, nctx, temps);
    }

    /// Run a number of sub-actions, perhaps inline (FIXME: needed to make temps an Object[] due to silly circular reference issues)
    APObject runsub(Action[] toEnqueue, APObject nctx, Object[] temps) {
        // run them, potentially inline
        if (_act.gctx.tp.shouldInline()) {
            foreach (act; toEnqueue) {
                act.state = ActionState.Running;
                act.run();

                // perhaps requeue it, or signal that it's done
                synchronized (act) {
                    switch (act.state) {
                        case ActionState.Running:
                            act.doneMutex.lock();
                            act.state = ActionState.Done;
                            act.doneCondition.notify();
                            act.doneMutex.unlock();
                            break;

                        case ActionState.Canceled:
                            _act.gctx.tp.enqueue([act]);
                            break;

                        case ActionState.Destroyed:
                            // ignore
                            break;

                        default:
                            synchronized (Stderr)
                                Stderr("Action ")(act.ast.toXML())(" in bad state for inline completion: ")(act.state).newline;
                            break;
                    }
                }
            }

        } else {
            _act.gctx.tp.enqueue(toEnqueue);

        }

        return nctx;
    }



    Object visit(PASTArguments node) {
        APObject ret = _act.ctx.getMember(_act, cast(ubyte[]) "\x1bargument");
        if (ret is null)
            ret = _act.gctx.nul;
        return ret;
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

    Object visit(PASTIntWidth node) {
      APObject ret = new APObject(_act, _act.ctx);
      ret.setInteger(_act, ptrdiff_t.sizeof);
      return ret;
    }

    Object visit(PASTVersion node) {
      throw new APUnimplementedException("PASTVersion");
    }

    Object visit(PASTParent node) {
      APObject obj = cast(APObject) node.a1.accept(this);
      return obj.getParent(_act);
    }

    APObject call_try(APObject func, APObject excptHandler, APObject arg) {
      // make sure it's really a function
      PASTNode fast = func.getAST(_act);
      if (fast is null) {
	throw new APInterpFailure("Called uninitialized thing.");
      }
      PASTProc fproc = cast(PASTProc) fast;
      if (fproc is null) {
	throw new APInterpFailure("Called a non-procedure.");
      }
      _act.setExcptHandler(excptHandler);
      
      // make the context for this call
      APObject nctx = new APObject(_act, func.getParent(_act));
      nctx.setMember(_act, cast(ubyte[]) "\x1bargument", arg);
      
      // and temps
      APAccessor[] temps;
      temps.length = fproc.temps;
      foreach (i, _; temps) {
	temps[i] = new APAccessor();
      }
      
      // and put them in a queue
      Action[] toEnqueue;
      toEnqueue.length = fproc.stmts.length;
      foreach (i, ast; fproc.stmts) {
	toEnqueue[i] = _act.createChild(ast, nctx, temps);
      }
      
      return runsub(toEnqueue, nctx, temps);
    }

    Object visit(PASTCatch node) {
        APObject arg = cast(APObject) node.a1.accept(this);
      APObject func = cast(APObject) node.a2.accept(this);
      APObject excptHandler = cast(APObject) node.a3.accept(this);
      PASTNode fast = func.getAST(_act);
      if (fast is null) {
	throw new APInterpFailure("Called uninitialized thing.");
      }
      PASTProc fproc = cast(PASTProc) fast;
      if (fproc is null) {
	throw new APInterpFailure("Called a non-procedure.");
      }
      return call_try(func, excptHandler, arg);
    }

    Object visit(PASTThrow node) {
      APObject excpt = cast(APObject) node.a1.accept(this);
      APObject handler = _act.findExcptHandler();

      // set up the commit action
      PASTThrowCommit ptc = new PASTThrowCommit();
      _act.addCommit(&ptc.commit);

      return call(handler, excpt);
    }

    Object visit(PASTArrayLength node) {
      APObject obj = cast(APObject) node.a1.accept(this);
      APObject ret = new APObject(_act, _act.ctx);
      ret.setInteger(_act, obj.getArrayLength(_act));
      return ret;
    }

    Object visit(PASTMembers node) {
      APObject obj = cast(APObject) node.a1.accept(this);
      APObject ret = new APObject(_act, _act.ctx);
      ubyte[][] names = obj.getMembers(_act);
      ret.setArrayLength(_act, names.length);
      foreach (i, name ; names) {
	APObject e = new APObject(_act, _act.ctx);
	e.setRaw(_act, name);
	ret.setArrayElement(_act, i, e);
      }
      return ret;
    }

    Object visit(PASTInteger node) {
      APObject obj = cast(APObject) node.a1.accept(this);
      ubyte[] raw = obj.getRaw(_act);
      ptrdiff_t val = 0;
      switch (raw.length) {
      case 1:
	val = raw[0];
	break;
	
      case 2:
	val =
	  (raw[0] << 8) |
	  (raw[1]);
	break;
	
      case 4:
	val =
	  (raw[0] << 24) |
	  (raw[1] << 16) |
	  (raw[2] << 8) |
	  (raw[3]);
	break;
        
      case 8:
	static if(ptrdiff_t.sizeof >= 8) {
	  val =
	    (cast(ptrdiff_t) raw[0] << 56) |
	    (cast(ptrdiff_t) raw[1] << 48) |
	    (cast(ptrdiff_t) raw[2] << 40) |
	    (cast(ptrdiff_t) raw[3] << 32) |
	    (cast(ptrdiff_t) raw[4] << 24) |
	    (cast(ptrdiff_t) raw[5] << 16) |
	    (cast(ptrdiff_t) raw[6] << 8) |
	    (cast(ptrdiff_t) raw[7]);
	} else {
	  val =
	    (raw[4] << 24) |
	    (raw[5] << 16) |
	    (raw[6] << 8) |
	    (raw[7]);
	}
	break;
      default:
	throw new APInterpFailure("Cannot create an integer from data of lengths other than 1, 2, 4, 8.");
      }
      APObject ret = new APObject(_act, obj.getParent(_act));
      ret.setInteger(_act, val);
      return ret;
    }

    Object visit(PASTByte node) {
      APObject obj = cast(APObject) node.a1.accept(this);
      ptrdiff_t i = obj.getInteger(_act);
      ubyte[] b;
      b[] = i % 256;
      APObject ret = new APObject(_act, _act.ctx);
      ret.setRaw(_act, b);
      return ret;
    }

    Object visit(PASTPrint node) {
        // FIXME: this should not print immediately
        APObject toprint = cast(APObject) node.a1.accept(this);

        // get the data
        ubyte[] raw = toprint.getRaw(_act);

        // and make the printing a commit action
        PASTPrintCommit ppc = new PASTPrintCommit(toprint, raw);
        _act.addCommit(&ppc.commit);

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

    Object visit(PASTConcat node) {
        APObject o1 = cast(APObject) node.a1.accept(this);
        APObject o2 = cast(APObject) node.a2.accept(this);
        APObject ret = new APObject(_act, _act.ctx);
        ret.setRaw(_act, o1.getRaw(_act) ~ o2.getRaw(_act));
        return ret;
    }

    Object visit(PASTWrap node) {
//       APObject o1 = cast(APObject) node.a1.accept(this);
//       APObject o2 = cast(APObject) node.a2.accept(this);
//       APObject ret = new APObject(_act, _act.ctx);
      
//       // Missing stuff
      
//       return ret;
      throw new APUnimplementedException("PASTWrap");
    }

    Object visit(PASTParentSet node) {
        // a1.parent = a2
        APObject trg = cast(APObject) node.a1.accept(this);
        APObject obj = cast(APObject) node.a2.accept(this);

        trg.setParent(_act, obj);

        return _act.gctx.nul;
    }

    Object visit(PASTArrayConcat node) {
      APObject o1 = cast(APObject) node.a1.accept(this);
      APObject o2 = cast(APObject) node.a2.accept(this);

      if (!o1.isArray(_act) || !o2.isArray(_act))
	throw new APInterpFailure("Operand of array concat not an array.");

      APObject ret = new APObject(_act, _act.ctx);
      uint n1 = o1.getArrayLength(_act);
      uint n2 = o2.getArrayLength(_act);

      for (uint i = 0; i < n1; ++i)
	ret.setArrayElement(_act, i, o1.getArrayElement(_act, i));
      for (uint i = 0; i < n2; ++i)
	ret.setArrayElement(_act, i+n1, o2.getArrayElement(_act, i));
      return ret;
    }

    Object visit(PASTArrayLengthSet node) {
      APObject array = cast(APObject) node.a1.accept(this);
      APObject len = cast(APObject) node.a2.accept(this);
      array.setArrayLength(_act, cast(uint) len.getInteger(_act));
      return _act.gctx.nul;
    }

    Object visit(PASTArrayIndex node) {
        // a1[a2]
        APObject obj = cast(APObject) node.a1.accept(this);
        APObject index = cast(APObject) node.a2.accept(this);

        APObject ret = obj.getArrayElement(_act, index.getInteger(_act));
        if (ret is null) {
            return _act.gctx.nul;
        } else {
            return ret;
        }
    }

    private void getIntegers(PASTBinary node, out ptrdiff_t i1, out ptrdiff_t i2) {
      APObject o1 = cast(APObject) node.a1.accept(this);
      APObject o2 = cast(APObject) node.a2.accept(this);
      i1 = o1.getInteger(_act);
      i2 = o2.getInteger(_act);
    }

    Object visit(PASTMul node) {
      ptrdiff_t i1, i2;
      getIntegers(node, i1, i2);
      APObject ret = new APObject(_act, _act.ctx);
      ret.setInteger(_act, i1 * i2);
      return ret;
    }

    Object visit(PASTDiv node) {
      ptrdiff_t i1, i2;
      getIntegers(node, i1, i2);
      APObject ret = new APObject(_act, _act.ctx);
      ret.setInteger(_act, i1 / i2);
      return ret;
    }

    Object visit(PASTMod node) {
      ptrdiff_t i1, i2;
      getIntegers(node, i1, i2);
      APObject ret = new APObject(_act, _act.ctx);
      ret.setInteger(_act, i1 % i2);
      return ret;
    }

    Object visit(PASTAdd node) {
      ptrdiff_t i1, i2;
      getIntegers(node, i1, i2);
      APObject ret = new APObject(_act, _act.ctx);
      ret.setInteger(_act, i1 + i2);
      return ret;
    }

    Object visit(PASTSub node) {
      ptrdiff_t i1, i2;
      getIntegers(node, i1, i2);
      APObject ret = new APObject(_act, _act.ctx);
      ret.setInteger(_act, i1 - i2);
      return ret;
    }

    Object visit(PASTSL node) {
      ptrdiff_t i1, i2;
      getIntegers(node, i1, i2);
      APObject ret = new APObject(_act, _act.ctx);
      ret.setInteger(_act, i1 << i2);
      return ret;
    }

    Object visit(PASTSR node) {
      ptrdiff_t i1, i2;
      getIntegers(node, i1, i2);
      APObject ret = new APObject(_act, _act.ctx);
      ret.setInteger(_act, i1 >> i2);
      return ret;
    }

    Object visit(PASTBitwiseAnd node) {
      ptrdiff_t i1, i2;
      getIntegers(node, i1, i2);
      APObject ret = new APObject(_act, _act.ctx);
      ret.setInteger(_act, i1 & i2);
      return ret;
    }

    Object visit(PASTBitwiseNAnd node) {
      ptrdiff_t i1, i2;
      getIntegers(node, i1, i2);
      APObject ret = new APObject(_act, _act.ctx);
      ret.setInteger(_act, ~(i1 & i2));
      return ret;
    }

    Object visit(PASTBitwiseOr node) {
      ptrdiff_t i1, i2;
      getIntegers(node, i1, i2);
      APObject ret = new APObject(_act, _act.ctx);
      ret.setInteger(_act, i1 | i2);
      return ret;
    }

    Object visit(PASTBitwiseNOr node) {
      ptrdiff_t i1, i2;
      getIntegers(node, i1, i2);
      APObject ret = new APObject(_act, _act.ctx);
      ret.setInteger(_act, ~(i1 | i2));
      return ret;
    }

    Object visit(PASTBitwiseXOr node) {
      ptrdiff_t i1, i2;
      getIntegers(node, i1, i2);
      APObject ret = new APObject(_act, _act.ctx);
      ret.setInteger(_act, i1 ^ i2);
      return ret;
    }

    Object visit(PASTBitwiseNXOr node) {
      ptrdiff_t i1, i2;
      getIntegers(node, i1, i2);
      APObject ret = new APObject(_act, _act.ctx);
      ret.setInteger(_act, ~(i1 ^ i2));
      return ret;
    }

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

    Object visit(PASTArrayIndexSet node) {
      APObject arr = cast(APObject) node.a1.accept(this);
      APObject index = cast(APObject) node.a2.accept(this);
      APObject value = cast(APObject) node.a3.accept(this);

      uint i = cast(uint) index.getInteger(_act);
      uint arr_len = arr.getArrayLength(_act);
      // There is a FIXME in APObject about this, but I took care
      // of it here instead... perhaps move into
      // APObject.setArrayElement(...)?
      if (i >= arr_len)
	arr.setArrayLength(_act, i+1);
      arr.setArrayElement(_act, i, value);
      return _act.gctx.nul;
    }

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

    Object visit(PASTIntCmp node) {
      APObject arg = cast(APObject) node.a1.accept(this);
      APObject int1 = cast(APObject) node.a2.accept(this);
      APObject int2 = cast(APObject) node.a3.accept(this);
      APObject f1 = cast(APObject) node.a4.accept(this);
      APObject f2 = cast(APObject) node.a5.accept(this);

      ptrdiff_t i1 = int1.getInteger(_act);
      ptrdiff_t i2 = int2.getInteger(_act);

      APObject fun;

      // Select the right function depending on the command and
      // the values.
      switch(node.cmd) {
      case psl_lt:
	fun = (i1 < i2) ? f1 : f2; break;
      case psl_lte:
	fun = (i1 <= i2) ? f1 : f2; break;
      case psl_gt:
	fun = (i1 > i2) ? f1 : f2; break;
      case psl_gte:
	fun = (i1 >= i2) ? f1 : f2; break;
      case psl_eq:
	fun = (i1 == i2) ? f1 : f2; break;
      case psl_ne:
	fun = (i1 != i2) ? f1 : f2; break;
      }

      return call(fun, arg);
    }

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

    Object visit(PASTLoop node) {
        Action[] toEnqueue;
        Action nact;

        // temporaries can't be shared across loop boundaries, so there's no reason to create conflicts in them
        APAccessor[] ntemps;
        ntemps.length = _act.temps.length;
        foreach (ti, _; ntemps) {
            ntemps[ti] = new APAccessor();
        }

        // get each of the statements
        PASTNode[] stmts = node.stmts;
        foreach (stmti, stmt; stmts) {
            if (stmti == stmts.length - 1) {
                // this is the last statement, so we need to result as the argument of the next iteration
                stmt = new PASTMemberSet(new PASTThis(), new PASTRaw(cast(ubyte[]) "\x1bargument"), stmt);
            }

            // now make the corresponding action
            nact = _act.createSibling(stmt, ntemps);
            if (nact is null) {
                // must have been canceled
                return _act.gctx.nul;
            }
            toEnqueue ~= nact;
        }

        // repeat ourself
        nact = _act.createSibling(node, ntemps);
        if (nact is null) {
            return _act.gctx.nul;
        }
        // in the queue to avoid recursion
        _act.gctx.tp.enqueue([nact]);

        // and run the steps
        runsub(toEnqueue, _act.ctx, cast(Object[]) ntemps);

        return _act.gctx.nul;
    }

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

/// Committer for PASTPrint
class PASTPrintCommit {
    this(APObject obj, ubyte[] data) {
        _obj = obj;
        _data = data.dup;
    }

    void commit(Action act) {
        synchronized (Stdout) {
            if (_data.length == 0) {
                Stdout(cast(void*) _obj).newline;
            } else {
                Stdout(cast(char[]) _data).newline;
            }
        }
    }

    private {
        APObject _obj;
        ubyte[] _data;
    }
}

/// Committer for PASTThrow
class PASTThrowCommit {
    void commit(Action act) {
        act.throwCancel();
    }
}
