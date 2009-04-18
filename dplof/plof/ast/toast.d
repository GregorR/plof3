/**
 * Simple translator from flat PSL to an AST-like tree (strictly not an AST
 * since the syntax is flat, but close enough)
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

module plof.ast.toast;

import tango.text.convert.Integer;
alias tango.text.convert.Integer.toString intToString;

import plof.psl.bignum;
import plof.psl.psl;

import plof.ast.ast;

/// Convert a PSL proc to an array of AST statements (a PASTProc)
PASTProc pslToAST(ubyte[] psl)
{
    // keep the current AST nodes on a stack
    PASTNode[] stack;

    // the first element on the stack is implied to be the arguments
    stack ~= new PASTArguments();

    // and the result
    PASTNode[] res;

    // if debugging is on we'll figure out our file and line/col
    char[] dsrcfile;
    int dsrcline = -1, dsrccol = -1;

    // number of the current temporary
    uint curTemp = 0;

    // some convenience functions
    PASTNode pop() {
        PASTNode ret;
        if (stack.length > 0) {
            ret = stack[$-1];
            stack.length = stack.length - 1;
        } else {
            ret = new PASTNull();
        }
        return ret;
    }
    void use1(void delegate(PASTNode) f) {
        PASTNode a = pop();
        f(a);
    }
    void use2(void delegate(PASTNode, PASTNode) f) {
        PASTNode b = pop();
        PASTNode a = pop();
        f(a, b);
    }
    void use3(void delegate(PASTNode, PASTNode, PASTNode) f) {
        PASTNode c = pop();
        PASTNode b = pop();
        PASTNode a = pop();
        f(a, b, c);
    }
    void use4(void delegate(PASTNode, PASTNode, PASTNode, PASTNode) f) {
        PASTNode d = pop();
        PASTNode c = pop();
        PASTNode b = pop();
        PASTNode a = pop();
        f(a, b, c, d);
    }
    void use5(void delegate(PASTNode, PASTNode, PASTNode, PASTNode, PASTNode) f) {
        PASTNode e = pop();
        PASTNode d = pop();
        PASTNode c = pop();
        PASTNode b = pop();
        PASTNode a = pop();
        f(a, b, c, d, e);
    }


    // this function guarantees that the stack's actions happen in the correct order
    void orderStack() {
        // go through the entire stack and (potentially) duplicate it
        for (uint si = 0; si < stack.length; si++) {
            if (stack[si].usesHeap()) {
                // put this in a temp and duplicate it
                uint stemp = curTemp++;
                res ~= new PASTTempSet(stemp, stack[si]);
                stack[si] = new PASTTempGet(stemp);
            }
        }
    }

    // now go through each of the operations
    for (uint i = 0; i < psl.length; i++) {
        ubyte cmd = psl[i];
        ubyte[] sub;

        if (cmd >= psl_marker) {
            // 0xFC through 0xFF have data, so get that too
            i++;
            uint len;
            i += pslBignumToInt(psl[i..$], len);
            sub = psl[i..i+len];
            i += len - 1;
        }

        switch (cmd) {
            case psl_push0:
            case psl_push1:
            case psl_push2:
            case psl_push3:
            case psl_push4:
            case psl_push5:
            case psl_push6:
            case psl_push7:
                // make sure the stack is at least deep enough
                if (stack.length < cmd + 1) {
                    // just a null
                    stack ~= new PASTNull();
                } else {
                    // since this could screw up the order of events, order the stack
                    if (stack[$-cmd-1].usesHeap()) {
                        orderStack();
                    }

                    // don't duplicate anything (that is, don't recreate new objects or such)
                    if (cast(PASTTempGet) stack[$-cmd-1] is null) {
                        uint stemp = curTemp++;
                        res ~= new PASTTempSet(stemp, stack[$-cmd-1]);
                        stack[$-cmd-1] = new PASTTempGet(stemp);
                    }

                    // then push the right one
                    stack ~= stack[$-cmd-1];
                }
                break;

            case psl_pop:
                if (stack.length > 0) {
                    // need to make sure these effects aren't lost
                    if (stack[$-1].hasEffects()) {
                        orderStack();
                    }

                    pop();
                }
                break;

            case psl_this:
                stack ~= new PASTThis();
                break;

            case psl_null:
                stack ~= new PASTNull();
                break;

            case psl_global:
                stack ~= new PASTGlobal();
                break;

            case psl_new:
                stack ~= new PASTNew();
                break;

            case psl_combine:
                use2((PASTNode a, PASTNode b) {
                    stack ~= new PASTCombine(a, b);
                });
                break;

            case psl_member:
                use2((PASTNode a, PASTNode b) {
                    stack ~= new PASTMember(a, b);
                });
                break;

            case psl_memberset:
                // doesn't push anything, so order the stack
                orderStack();
                use3((PASTNode a, PASTNode b, PASTNode c) {
                    res ~= new PASTMemberSet(a, b, c);
                });
                break;

            case psl_parent:
                use1((PASTNode a) {
                    stack ~= new PASTParent(a);
                });
                break;

            case psl_parentset:
                // doesn't push anything, so order the stack
                orderStack();
                use2((PASTNode a, PASTNode b) {
                    res ~= new PASTParentSet(a, b);
                });
                break;

            case psl_call:
                use2((PASTNode args, PASTNode func) {
                    stack ~= new PASTCall(func, args);
                });
                break;

            case psl_return:
                // just make sure nothing else happens
                i = psl.length;
                break;

            case psl_throw:
                /* since this leaves nothing on the stack, need to make sure
                 * stack things are done in the proper order */
                orderStack();
                use1((PASTNode a) {
                    res ~= new PASTThrow(a);
                });
                break;

            case psl_catch:
                use3((PASTNode arg, PASTNode a, PASTNode b) {
                    stack ~= new PASTCatch(arg, a, b);
                });
                break;

            case psl_cmp:
                use2((PASTNode a, PASTNode b) {
                    stack ~= new PASTCmp(a, b);
                });
                break;

            case psl_concat:
                use2((PASTNode a, PASTNode b) {
                    stack ~= new PASTConcat(a, b);
                });
                break;

            case psl_wrap:
                use2((PASTNode towrap, PASTNode wrapin) {
                    stack ~= new PASTWrap(towrap, wrapin);
                });
                break;

            case psl_resolve:
                // get everything in order since this pushes two temps
                orderStack();
                use2((PASTNode a, PASTNode b) {
                    uint temp1 = curTemp++;
                    uint temp2 = curTemp++;

                    // resolve creates the two temps
                    res ~= new PASTResolve(a, b, temp1, temp2);
                    stack ~= new PASTTempGet(temp1);
                    stack ~= new PASTTempGet(temp2);
                });
                break;

            case psl_while:
                use3((PASTNode arg, PASTNode cond, PASTNode code) {
                    stack ~= new PASTWhile(arg, cond, code);
                });
                break;

            case psl_replace:
                throw new PASTFailure("Replace is unsupported.");
                break;

            case psl_if:
                use4((PASTNode arg, PASTNode a, PASTNode ift, PASTNode iff) {
                    stack ~= new PASTIf(arg, a, ift, iff);
                });
                break;

            case psl_array:
                // we restrict this to arrays of a known size
                use1((PASTNode pnsize) {
                    PASTNativeInteger pnisize = cast(PASTNativeInteger) pnsize;
                    if (pnisize is null) {
                        throw new PASTFailure("Array of unknown length.");
                    } else {
                        // make sure the stack is large enough
                        uint size = pnisize.value;
                        while (stack.length < size) {
                            stack = [new PASTNull()] ~ stack;
                        }

                        // then make the top of it into a PASTArray
                        PASTArray parr = new PASTArray(stack[$-size .. $].dup);
                        stack.length = stack.length - size;
                        stack ~= parr;
                    }
                });
                break;

            case psl_aconcat:
                use2((PASTNode a, PASTNode b) {
                    stack ~= new PASTArrayConcat(a, b);
                });
                break;

            case psl_length:
                use1((PASTNode a) {
                    stack ~= new PASTArrayLength(a);
                });
                break;

            case psl_lengthset:
                // doesn't push anything, so order the stack
                orderStack();
                use2((PASTNode arr, PASTNode len) {
                    res ~= new PASTArrayLengthSet(arr, len);
                });
                break;

            case psl_index:
                use2((PASTNode arr, PASTNode i) {
                    stack ~= new PASTArrayIndex(arr, i);
                });
                break;

            case psl_indexset:
                // doesn't push anything, so order the stack
                orderStack();
                use3((PASTNode arr, PASTNode i, PASTNode v) {
                    res ~= new PASTArrayIndexSet(arr, i, v);
                });
                break;

            case psl_members:
                use1((PASTNode a) {
                    stack ~= new PASTMembers(a);
                });
                break;

            case psl_rawlength:
                use1((PASTNode a) {
                    stack ~= new PASTRawLength(a);
                });
                break;

            case psl_slice:
                use3((PASTNode a, PASTNode b, PASTNode c) {
                    stack ~= new PASTSlice(a, b, c);
                });
                break;

            case psl_rawcmp:
                use2((PASTNode a, PASTNode b) {
                    stack ~= new PASTRawCmp(a, b);
                });
                break;

            case psl_integer:
                use1((PASTNode pnfrom) {
                    // optimize this if possible
                    PASTRaw prfrom = cast(PASTRaw) pnfrom;
                    if (prfrom !is null) {
                        // figure out the real value
                        uint val;
                        switch (prfrom.data.length) {
                            case 1:
                                val = prfrom.data[0];
                                break;

                            case 2:
                                val =
                                    (prfrom.data[0] << 8) |
                                    (prfrom.data[1]);
                                break;

                            case 4:
                                val =
                                    (prfrom.data[0] << 24) |
                                    (prfrom.data[1] << 16) |
                                    (prfrom.data[2] << 8) |
                                    (prfrom.data[3]);
                                break;

                            case 8:
                                static if(ptrdiff_t.sizeof >= 8) {
                                    val =
                                        (cast(ptrdiff_t) prfrom.data[0] << 56) |
                                        (cast(ptrdiff_t) prfrom.data[1] << 48) |
                                        (cast(ptrdiff_t) prfrom.data[2] << 40) |
                                        (cast(ptrdiff_t) prfrom.data[3] << 32) |
                                        (cast(ptrdiff_t) prfrom.data[4] << 24) |
                                        (cast(ptrdiff_t) prfrom.data[5] << 16) |
                                        (cast(ptrdiff_t) prfrom.data[6] << 8) |
                                        (cast(ptrdiff_t) prfrom.data[7]);
                                } else {
                                    val =
                                        (prfrom.data[4] << 24) |
                                        (prfrom.data[5] << 16) |
                                        (prfrom.data[6] << 8) |
                                        (prfrom.data[7]);
                                }
                                break;

                            default:
                                throw new PASTFailure("Cannot create an integer from data of lengths other than 1, 2, 4, 8.");
                                break;
                        }
                        
                        // then push it as a native integer
                        stack ~= new PASTNativeInteger(val);

                    } else {
                        // we don't know anything about the integer, but do our best
                        stack ~= new PASTInteger(pnfrom);

                    }
                });
                break;

            case psl_intwidth:
                stack ~= new PASTIntWidth();
                break;

            case psl_mul:
                use2((PASTNode a, PASTNode b) {
                    // FIXME: could do constant folding
                    stack ~= new PASTMul(a, b);
                });
                break;

            case psl_div:
                use2((PASTNode a, PASTNode b) {
                    // FIXME: could do constant folding
                    stack ~= new PASTDiv(a, b);
                });
                break;

            case psl_mod:
                use2((PASTNode a, PASTNode b) {
                    // FIXME: could do constant folding
                    stack ~= new PASTMod(a, b);
                });
                break;
    
            case psl_add:
                use2((PASTNode a, PASTNode b) {
                    // FIXME: could do constant folding
                    stack ~= new PASTAdd(a, b);
                });
                break;
  
            case psl_sub:
                use2((PASTNode a, PASTNode b) {
                    // FIXME: could do constant folding
                    stack ~= new PASTSub(a, b);
                });
                break;

            case psl_lt:
            case psl_lte:
            case psl_eq:
            case psl_ne:
            case psl_gt:
            case psl_gte: // integer comparisons
                use2((PASTNode a, PASTNode b) {
                    stack ~= new PASTIntCmp(a, b, cmd);
                });
                break;

            case psl_sl:
                use2((PASTNode a, PASTNode b) {
                    // FIXME: could do constant folding
                    stack ~= new PASTSL(a, b);
                });
                break;

            case psl_sr:
                use2((PASTNode a, PASTNode b) {
                    // FIXME: could do constant folding
                    stack ~= new PASTSR(a, b);
                });
                break;

            case psl_or:
                use2((PASTNode a, PASTNode b) {
                    // FIXME: could do constant folding
                    stack ~= new PASTBitwiseOr(a, b);
                });
                break;

            case psl_nor:
                use2((PASTNode a, PASTNode b) {
                    // FIXME: could do constant folding
                    stack ~= new PASTBitwiseNOr(a, b);
                });
                break;

            case psl_xor:
                use2((PASTNode a, PASTNode b) {
                    // FIXME: could do constant folding
                    stack ~= new PASTBitwiseXOr(a, b);
                });
                break;

            case psl_nxor:
                use2((PASTNode a, PASTNode b) {
                    // FIXME: could do constant folding
                    stack ~= new PASTBitwiseNXOr(a, b);
                });
                break;

            case psl_and:
                use2((PASTNode a, PASTNode b) {
                    // FIXME: could do constant folding
                    stack ~= new PASTBitwiseAnd(a, b);
                });
                break;

            case psl_nand:
                use2((PASTNode a, PASTNode b) {
                    // FIXME: could do constant folding
                    stack ~= new PASTBitwiseNAnd(a, b);
                });
                break;

            case psl_byte:
                use1((PASTNode pnval) {
                    // as an optimization, support NativeIntegers directly
                    PASTNativeInteger pnival = cast(PASTNativeInteger) pnval;
                    if (pnival !is null) {
                        // just get the value directly
                        stack ~= new PASTRaw([cast(ubyte) pnival.value]);
                    } else {
                        // dynamic
                        stack ~= new PASTByte(pnval);
                    }
                });
                break;

            case psl_float:
            case psl_fint:
            case psl_fmul:
            case psl_fdiv:
            case psl_fmod:
            case psl_fadd:
            case psl_fsub:
            case psl_flt:
            case psl_flte:
            case psl_feq:
            case psl_fne:
            case psl_fgt:
            case psl_fgte:
                throw new PASTFailure("Floating point code is unsupported.");
                break;

            case psl_version:
                stack ~= new PASTVersion();
                break;

            case psl_dlopen:
            case psl_dlclose:
            case psl_dlsym:
            case psl_cmalloc:
            case psl_cfree:
            case psl_cget:
            case psl_cset:
            case psl_ctype:
            case psl_cstruct:
            case psl_csizeof:
            case psl_prepcif:
            case psl_ccall:
                throw new PASTFailure("FFI code is unsupported.");
                break;

            case psl_dsrcfile:
                use1((PASTNode sf) {
                    PASTRaw rd = cast(PASTRaw) sf;
                    if (rd !is null && dsrcfile.length == -1) {
                        /* don't care about the case where it's pretending to
                         * be anything else */
                        dsrcfile = cast(char[]) rd.data;
                    }
                });
                break;

            case psl_dsrcline:
                use1((PASTNode sl) {
                    PASTNativeInteger ni = cast(PASTNativeInteger) sl;
                    if (ni !is null && dsrcline == -1) {
                        dsrcline = ni.value;
                    }
                });
                break;

            case psl_dsrccol:
                use1((PASTNode sc) {
                    PASTNativeInteger ni = cast(PASTNativeInteger) sc;
                    if (ni !is null && dsrccol == -1) {
                        dsrccol = ni.value;
                    }
                });
                break;

            case psl_print:
                // doesn't return anything
                orderStack();
                use1((PASTNode a) {
                    res ~= new PASTPrint(a);
                });
                break;

            case psl_debug:
                // nothing
                break;

            case psl_include:
                throw new PASTFailure("Runtime include not supported.");

            case psl_parse:
            case psl_gadd:
            case psl_grem:
            case psl_gcommit:
            case psl_marker:
                throw new PASTFailure("Grammar engine not supported.");

            case psl_immediate:
                // Just do it now as a call
                //stack ~= new PASTCall(new PASTNull(), pslToAST(sub));
                break;

            case psl_code:
                // translate to a PASTProc
                stack ~= pslToAST(sub);
                break;

            case psl_raw:
                // simply raw data
                stack ~= new PASTRaw(sub);
                break;

            default:
                throw new PASTFailure("Unrecognized operation " ~ intToString(cmd) ~ ".");
        }
    }

    // now make sure anything in the stack gets done
    res ~= stack;

    // and turn it into a proc
    return new PASTProc(res, curTemp, dsrcfile, dsrcline, dsrccol);
}

/// Translation failure
class PASTFailure : Exception {
    this(char[] msg) { super(msg); }
}
