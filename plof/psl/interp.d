/**
 * The core PSL interpreter
 *
 * License:
 *  Copyright (c) 2007, 2008  Gregor Richards
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

module plof.psl.interp;

import tango.io.Stdout;

import tango.stdc.stdlib;

import plof.prp.iprp;

import plof.psl.bignum;
import plof.psl.gc;
import plof.psl.pslobject;
import plof.psl.treemap;

/// The stack, allocated as a PlofGCable for easy mark/sweeping
struct PSLStack {
    PlofGCable gc;

    static PSLStack* allocate()
    {
        PSLStack* ret = PlofGCable.allocate!(PSLStack)();
        ret.gc.foreachRef = &ret.foreachRef;
        return ret;
    }

    /// Foreach over elements in the stack, mainly useful as master mark
    void foreachRef(void delegate(PlofGCable*) callback)
    {
        foreach (i; stack) {
            callback(cast(PlofGCable*) i);
        }
    }

    /// Truncate the stack to the given length
    void truncate(uint len)
    {
        if (len < stack.length) {
            foreach (i; stack[len..$]) {
                i.gc.refDown();
            }
            stack.length = len;
        }
    }

    // The stack itself
    PSLObject*[] stack;
}

/// Interpret the given code, with the given grammar parser for parser commands
void interpret(ubyte[] psl, bool immediate = false, IPlofRuntimeParser prp = null)
{
    // Create a stack,
    PSLStack* stack = PSLStack.allocate();
    stack.gc.blessUp();

    // and a master context,
    PSLObject* context = PSLObject.allocate(pslNull);
    context.gc.blessUp();

    // and run it
    interpret(psl, stack, context, immediate, prp);

    // then get rid of the context and stack
    context.gc.blessDown();
    stack.gc.blessDown();
}

/// Interpret the given code in the given context
PSLObject* interpret(ubyte[] psl, PSLStack* stack, PSLObject* context,
                     bool immediate = false, IPlofRuntimeParser prp = null)
{
    // keep track of the topmost element of the stack we didn't push
    int lastlen = stack.stack.length;

    // for debugging purposes, the filename, line and column
    char[] dfile;
    int dline = -1, dcol = -1;

    // some convenience functions
    void push(PSLObject* x) {
        stack.stack ~= x;
        x.gc.refUp();
    }
    void checkLL()
    {
        if (stack.stack.length < lastlen) {
            lastlen = stack.stack.length;
        }
    }
    void pop() {
        if (stack.stack.length > 0) {
            stack.stack[$-1].gc.refDown();
            stack.stack.length = stack.stack.length - 1;
            checkLL();
        }
    }
    PSLObject* popBless() {
        PSLObject* r;
        if (stack.stack.length > 0) {
            r = stack.stack[$-1];
            stack.stack.length = stack.stack.length - 1;
            checkLL();
            r.gc.blessUp();
            r.gc.refDown();
        } else {
            r = pslNull;
            r.gc.blessUp();
        }
        return r;
    }
    void use1(void delegate(PSLObject*) f) {
        PSLObject* a = popBless();
        f(a);
        a.gc.blessDown();
    }
    void use2(void delegate(PSLObject*, PSLObject*) f) {
        PSLObject* b = popBless();
        PSLObject* a = popBless();
        f(a, b);
        a.gc.blessDown();
        b.gc.blessDown();
    }
    void use3(void delegate(PSLObject*, PSLObject*, PSLObject*) f) {
        PSLObject* c = popBless();
        PSLObject* b = popBless();
        PSLObject* a = popBless();
        f(a, b, c);
        a.gc.blessDown();
        b.gc.blessDown();
        c.gc.blessDown();
    }
    void use4(void delegate(PSLObject*, PSLObject*, PSLObject*, PSLObject*) f) {
        PSLObject* d = popBless();
        PSLObject* c = popBless();
        PSLObject* b = popBless();
        PSLObject* a = popBless();
        f(a, b, c, d);
        a.gc.blessDown();
        b.gc.blessDown();
        c.gc.blessDown();
        d.gc.blessDown();
    }
    PSLObject* call(PSLObject* incontext, ubyte[] npsl) {
        // Create a context
        PSLObject* ncontext = PSLObject.allocate(incontext);
        ncontext.gc.blessUp();

        // then call
        PSLObject* thrown = interpret(npsl, stack, ncontext, false, prp);

        // the interpretation could have popped, so check our lastlen
        checkLL();

        // and get rid of the context
        ncontext.gc.blessDown();

        return thrown;
    }

    // NOTE: no nested templates, so we have to double-define this

    // integer operation over 2 args
    void intop(ptrdiff_t delegate(ptrdiff_t, ptrdiff_t) op) {
        use2((PSLObject* a, PSLObject* b) {
            // get the left and right values
            ptrdiff_t left, right, ret;
            if (!a.isArray && a.raw !is null &&
                a.raw.data.length == ptrdiff_t.sizeof) {
                left = *(cast(ptrdiff_t*) a.raw.data.ptr);
            } else {
                throw new InterpreterFailure("Expected int operands.");
            }
            if (!b.isArray && b.raw !is null &&
                b.raw.data.length == ptrdiff_t.sizeof) {
                right = *(cast(ptrdiff_t*) b.raw.data.ptr);
            } else {
                throw new InterpreterFailure("Expected int operands.");
            }

            // perform the op
            ret = op(left, right);

            // put that in raw data
            PSLRawData* rd = PSLRawData.allocate(
                (cast(ubyte*) &ret)[0..ptrdiff_t.sizeof]);

            // and in an object
            PSLObject* no = PSLObject.allocate(context);
            no.raw = rd;

            // then push it
            push(no);
        });
    }

    // integer comparison operation over 2 args
    PSLObject* intcmp(bool delegate(ptrdiff_t, ptrdiff_t) op) {
        PSLObject* thrown;

        use4((PSLObject* a, PSLObject* b, PSLObject* procy, PSLObject* procn) {
            // get the left and right values
            ptrdiff_t left, right;
            bool res;
            if (!a.isArray && a.raw !is null &&
                a.raw.data.length == ptrdiff_t.sizeof) {
                left = *(cast(ptrdiff_t*) a.raw.data.ptr);
            } else {
                throw new InterpreterFailure("Expected int operands.");
            }
            if (!b.isArray && b.raw !is null &&
                b.raw.data.length == ptrdiff_t.sizeof) {
                right = *(cast(ptrdiff_t*) b.raw.data.ptr);
            } else {
                throw new InterpreterFailure("Expected int operands.");
            }

            // perform the op
            res = op(left, right);

            // perform the procedure
            if (res) {
                if (!procy.isArray && procy.raw !is null) {
                    thrown = call(procy.parent, procy.raw.data);
                } else {
                    throw new InterpreterFailure("Expected procedure as argument to integer comparison.");
                }
            } else {
                if (!procn.isArray && procn.raw !is null) {
                    thrown = call(procy.parent, procn.raw.data);
                } else {
                    throw new InterpreterFailure("Expected procedure as argument to integer comparison.");
                }
            }
        });

        return thrown;
    }

    // float operation over 2 args
    void floatop(real delegate(real, real) op) {
        use2((PSLObject* a, PSLObject* b) {
            // get the left and right values
            real left, right, ret;
            if (!a.isArray && a.raw !is null &&
                a.raw.data.length == real.sizeof) {
                left = *(cast(real*) a.raw.data.ptr);
            } else {
                throw new InterpreterFailure("Expected floating point operands.");
            }
            if (!b.isArray && b.raw !is null &&
                b.raw.data.length == real.sizeof) {
                right = *(cast(real*) b.raw.data.ptr);
            } else {
                throw new InterpreterFailure("Expected floating point operands.");
            }

            // perform the op
            ret = op(left, right);

            // put that in raw data
            PSLRawData* rd = PSLRawData.allocate(
                (cast(ubyte*) &ret)[0..real.sizeof]);

            // and in an object
            PSLObject* no = PSLObject.allocate(context);
            no.raw = rd;

            // then push it
            push(no);
        });
    }

    // float comparison operation over 2 args
    PSLObject* floatcmp(bool delegate(real, real) op) {
        PSLObject* thrown;

        use4((PSLObject* a, PSLObject* b, PSLObject* procy, PSLObject* procn) {
            // get the left and right values
            real left, right;
            bool res;
            if (!a.isArray && a.raw !is null &&
                a.raw.data.length == real.sizeof) {
                left = *(cast(real*) a.raw.data.ptr);
            } else {
                throw new InterpreterFailure("Expected floating point operands.");
            }
            if (!b.isArray && b.raw !is null &&
                b.raw.data.length == real.sizeof) {
                right = *(cast(real*) b.raw.data.ptr);
            } else {
                throw new InterpreterFailure("Expected floating point operands.");
            }

            // perform the op
            res = op(left, right);

            // perform the procedure
            if (res) {
                if (!procy.isArray && procy.raw !is null) {
                    thrown = call(procy.parent, procy.raw.data);
                } else {
                    throw new InterpreterFailure("Expected procedure as argument to floating point comparison.");
                }
            } else {
                if (!procn.isArray && procn.raw !is null) {
                    thrown = call(procy.parent, procn.raw.data);
                } else {
                    throw new InterpreterFailure("Expected procedure as argument to floating point comparison.");
                }
            }
        });

        return thrown;
    }

    try {
        // simply go element-by-element, interpreting
        for (uint i = 0; i < psl.length; i++) {
            ubyte cmd = psl[i];
            ubyte[] sub;
    
            if (cmd >= 0xFE) {
                // 0xFE and 0xFF have data, so get that too
                i++;
                uint len;
                i += pslBignumToInt(psl[i..$], len);
                sub = psl[i..i+len];
                i += len - 1;
            }
    
            // if immediate, only run immediate commands
            if (immediate) {
                if (cmd == 0xFE) { // immediate
                    call(context, sub);
                }
                continue;
            }
    
            // now just switch off on the action
            switch (cmd) {
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                case 0x04:
                case 0x05:
                case 0x06:
                case 0x07: // push0-push7
                    if (stack.stack.length < cmd + 1) {
                        // just push a null
                        push(pslNull);
                    } else {
                        // push the appropriate object
                        push(stack.stack[$-cmd-1]);
                    }
                    break;
    
                case 0x08: // pop
                    pop();
                    break;
    
                case 0x09: // this
                    push(context);
                    break;
    
                case 0x0A: // null
                    push(pslNull);
                    break;
    
                case 0x0B: // global
                    push(pslGlobal);
                    break;
    
                case 0x0C: // new
                {
                    PSLObject* no = PSLObject.allocate(context);
                    push(no);
                    break;
                }
    
                case 0x0D: // combine
                    use2((PSLObject* a, PSLObject* b) {
                        PSLObject* no = PSLObject.allocate(context);
    
                        // add elements of a, b
                        a.members.foreachVal((char[] name, PlofGCable* val) {
                            no.members.add(name, val);
                        });
                        b.members.foreachVal((char[] name, PlofGCable* val) {
                            no.members.add(name, val);
                        });
    
                        // combine the raw or array data
                        if (a.isArray && b.isArray) {
                            no.arr = PSLArray.allocate(a.arr.arr, b.arr.arr);
    
                        } else {
                            // at least not BOTH arrays, check if one is empty
                            if (a.isArray && b.raw is null) {
                                no.arr = PSLArray.allocate(a.arr.arr);
    
                            } else if (b.isArray && a.raw is null) {
                                no.arr = PSLArray.allocate(b.arr.arr);
    
                            } else if (a.raw !is null) {
                                if (b.raw !is null) {
                                    no.raw = PSLRawData.allocate(a.raw.data, b.raw.data);
    
                                } else {
                                    no.raw = PSLRawData.allocate(a.raw.data);
    
                                }
    
                            } else if (b.raw !is null) {
                                no.raw = PSLRawData.allocate(b.raw.data);
    
                            }
                        }
    
                        // then push the new one
                        push(no);
                    });
                    break;
    
                case 0x0E: // member
                    use2((PSLObject* a, PSLObject* b) {
                        if (b.isArray || b.raw is null) {
                            // bad choice
                            push(pslNull);
    
                        } else {
                            PSLObject* o = cast(PSLObject*)
                                a.members.get(cast(char[]) b.raw.data);
                            if (o is null) {
                                push(pslNull);
                            } else {
                                push(o);
                            }
                        }
                    });
                    break;
    
                case 0x0F: // memberset
                    use3((PSLObject* a, PSLObject* b, PSLObject* c) {
                        if (!b.isArray && b.raw !is null) {
                            a.members.add(
                                cast(char[]) b.raw.data,
                                cast(PlofGCable*) c);
                        } else {
                            throw new InterpreterFailure("memberset's second parameter must be raw data.");
                        }
                    });
                    break;
    
                case 0x10: // parent
                    use1((PSLObject* a) {
                        push(a.parent);
                    });
                    break;
    
                case 0x11: // parentset
                    use2((PSLObject* a, PSLObject* b) {
                        a.parent = b;
                    });
                    break;
    
                case 0x12: // call
                {
                    PSLObject* thrown;
                    use1((PSLObject* a) {
                        if (!a.isArray && a.raw !is null) {
                            thrown = call(a.parent, a.raw.data);
                        } else {
                            throw new InterpreterFailure("call expects a procedure operand.");
                        }
                    });
    
                    // if it threw, die
                    if (thrown !is null) {
                        stack.truncate(lastlen);
                        return thrown;
                    }
                    break;
                }
    
                case 0x13: // return
                    return null; // well that was obvious
    
                case 0x14: // throw
                {
                    // get the object to throw,
                    PSLObject* tothrow = popBless();
                    
                    // then truncate the stack
                    stack.truncate(lastlen);
    
                    // and throw it
                    return tothrow;
                }
    
                case 0x15: // catch
                {
                    PSLObject* thrown;
    
                    use2((PSLObject* a, PSLObject* b) {
                        ubyte[] proca, procb;
                        if (!a.isArray && a.raw !is null) {
                            proca = a.raw.data;
                        } else {
                            throw new InterpreterFailure("catch expects procedure operands.");
                        }
                        if (!b.isArray && b.raw !is null) {
                            procb = b.raw.data;
                        } else {
                            throw new InterpreterFailure("catch expects procedure operands.");
                        }
    
                        thrown = call(a.parent, proca);
    
                        if (thrown !is null) {
                            // it threw, so call b
                            push(thrown);
    
                            // unbless the reference
                            thrown.gc.blessDown();
    
                            // then continue
                            thrown = call(b.parent, procb);
                        }
                    });
    
                    if (thrown !is null) {
                        // something was thrown that was not cought, so throw it further
                        stack.truncate(lastlen);
                        return thrown;
                    }
    
                    break;
                }
    
                case 0x16: // cmp
                {
                    PSLObject* thrown;
                    
                    use4((PSLObject* a, PSLObject* b, PSLObject* procy, PSLObject* procn) {
                        if (a is b) {
                            if (!procy.isArray && procy.raw !is null) {
                                thrown = call(procy.parent, procy.raw.data);
                            } else {
                                throw new InterpreterFailure("cmp expects procedure operands.");
                            }
                        } else {
                            if (!procn.isArray && procn.raw !is null) {
                                thrown = call(procn.parent, procn.raw.data);
                            } else {
                                throw new InterpreterFailure("cmp expects procedure operands.");
                            }
                        }
                    });
    
                    if (thrown !is null) {
                        // one side or the other threw, so throw
                        stack.truncate(lastlen);
                        return thrown;
                    }
                    break;
                }
    
                case 0x17: // concat
                    use2((PSLObject* a, PSLObject* b) {
                        // get the left and right data
                        ubyte[] left, right;
                        if (!a.isArray && a.raw !is null) {
                            left = a.raw.data;
                        } else {
                            throw new InterpreterFailure("concat expects raw data operands.");
                        }
                        if (!b.isArray && b.raw !is null) {
                            right = b.raw.data;
                        } else {
                            throw new InterpreterFailure("concat expects raw data operands.");
                        }
                        
                        // then use that to create raw data
                        PSLRawData* rd = PSLRawData.allocate(left, right);
                        
                        // and put that in a new object
                        PSLObject* no = PSLObject.allocate(context);
                        no.raw = rd;
    
                        // then push the new object
                        push(no);
                    });
                    break;
                    
                case 0x18: // wrap
                case 0x19: // iwrap
                    use1((PSLObject* a) {
                        // extract the raw data
                        ubyte[] data, outdata;
                        if (!a.isArray && a.raw !is null) {
                            data = a.raw.data;
                        } else {
                            throw new InterpreterFailure("wrap expects a raw data operand.");
                        }
    
                        // allocate enough space to store it all
                        uint bnlen = bignumBytesReq(data.length);
                        outdata.length = bnlen + data.length + 1;
    
                        // store the necessary data
                        if (cmd == 0x18) {
                            outdata[0] = 0xFF; // raw
                        } else {
                            outdata[0] = 0xFE; // immediate
                        }
                        intToPSLBignum(data.length, outdata[1..bnlen+1]);
                        outdata[bnlen+1..$] = data[];
    
                        // then put it all in raw data
                        PSLRawData* rd = PSLRawData.allocate(outdata);
    
                        // and put that in an object
                        PSLObject* no = PSLObject.allocate(context);
                        no.raw = rd;
    
                        // and push it
                        push(no);
                    });
                    break;
    
                case 0x1A: // loop
                    // simple
                    i = -1;
                    break;
    
                case 0x20: // array
                    use1((PSLObject* olen) {
                        // get the real length out of the object
                        uint len;
                        if (!olen.isArray && olen.raw !is null &&
                            olen.raw.data.length == ptrdiff_t.sizeof) {
                            len = cast(uint) *(cast(ptrdiff_t*) olen.raw.data.ptr);
                        } else {
                            throw new InterpreterFailure("array expects an integer operand.");
                        }
    
                        // then make an array with that many elements
                        PSLArray* arr = PSLArray.allocate(stack.stack[$-len..$]);
    
                        // and put it in an object
                        PSLObject* no = PSLObject.allocate(context);
                        no.arr = arr;
    
                        // remove them from the stack
                        for (; len > 0; len--) {
                            pop();
                        }
    
                        // and push the object
                        push(no);
                    });
                    break;
    
                case 0x21: // aconcat
                    use2((PSLObject* a, PSLObject* b) {
                        // get the left and right arrays
                        PSLObject*[] left, right;
                        if (a.isArray) {
                            left = a.arr.arr;
                        } else {
                            throw new InterpreterFailure("aconcat expects array operands.");
                        }
                        if (b.isArray) {
                            right = b.arr.arr;
                        } else {
                            throw new InterpreterFailure("aconcat expects array operands.");
                        }
    
                        // make a new array
                        PSLArray* arr = PSLArray.allocate(left, right);
    
                        // and an object to put it in
                        PSLObject* no = PSLObject.allocate(context);
                        no.arr = arr;
    
                        // then push that object
                        push(no);
                    });
                    break;
    
                case 0x22: // length
                    use1((PSLObject* a) {
                        // get the array itself
                        PSLObject*[] arr;
                        if (a.isArray) {
                            arr = a.arr.arr;
                        } else {
                            throw new InterpreterFailure("length expects an array operand.");
                        }
    
                        // make a new raw data object with its length
                        ptrdiff_t len = cast(ptrdiff_t) arr.length;
                        PSLRawData* rd = PSLRawData.allocate(
                            (cast(ubyte*) &len)[0..ptrdiff_t.sizeof]);
    
                        // and put it in an object
                        PSLObject* no = PSLObject.allocate(context);
                        no.raw = rd;
    
                        // and push it
                        push(no);
                    });
                    break;
    
                case 0x23: // lengthset
                    use2((PSLObject* a, PSLObject* b) {
                        // if the left object isn't an array, this makes no sense
                        if (!a.isArray || b.isArray || b.raw is null ||
                            b.raw.data.length != ptrdiff_t.sizeof) {
                            throw new InterpreterFailure("lengthset expects an array operand and an integer operand.");
                        }
    
                        // get the new length
                        ptrdiff_t len = *(cast(ptrdiff_t*) b.raw.data.ptr);
    
                        // and set it
                        a.arr.setLength(len);
                    });
                    break;
    
                case 0x24: // index
                    use2((PSLObject* a, PSLObject* b) {
                        // get the array,
                        PSLObject*[] arr;
                        if (a.isArray) {
                            arr = a.arr.arr;
                        } else {
                            throw new InterpreterFailure("index expects an array operand.");
                        }
    
                        // and the index
                        ptrdiff_t i;
                        if (!b.isArray && b.raw !is null &&
                            b.raw.data.length == ptrdiff_t.sizeof) {
                            i = *(cast(ptrdiff_t*) b.raw.data.ptr);
                        } else {
                            throw new InterpreterFailure("index expects an integer operand.");
                        }
    
                        // then push the object
                        if (i >= arr.length) {
                            push(pslNull);
                        } else {
                            push(arr[i]);
                        }
                    });
                    break;
    
                case 0x25: // indexset
                    use3((PSLObject* a, PSLObject* b, PSLObject* c) {
                        // if this isn't an array, bail out
                        if (!a.isArray)
                            throw new InterpreterFailure("indexset expects an array operand.");
    
                        // get the index
                        ptrdiff_t i;
                        if (!b.isArray && b.raw !is null &&
                            b.raw.data.length == ptrdiff_t.sizeof) {
                            i = *(cast(ptrdiff_t*) b.raw.data.ptr);
                        } else {
                            throw new InterpreterFailure("indexset expects an integer operand.");
                        }
    
                        // may need to reallocate it,
                        if (i >= a.arr.arr.length) {
                            a.arr.setLength(i + 1);
                        }
    
                        // finally, set the index
                        PSLObject* oldai = a.arr.arr[i];
                        a.arr.arr[i] = c;
                        c.gc.refUp();
                        oldai.gc.refDown();
                    });
                    break;
    
                case 0x26: // members
                    use1((PSLObject* a) {
                        PSLObject*[] members;
    
                        // go through all the members ...
                        a.members.foreachVal((char[] name, PlofGCable* ignore) {
                            PSLObject* no = PSLObject.allocate(context);
                            no.raw = PSLRawData.allocate(cast(ubyte[]) name);
                            members ~= no;
                        });
    
                        // then make the resultant object
                        PSLObject* no = PSLObject.allocate(context);
                        no.arr = PSLArray.allocate(members);
                       
                        // and push it 
                        push(no);
                    });
                    break;
    
                case 0x70: // integer
                    use1((PSLObject* a) {
                        ptrdiff_t val = 0;
    
                        // if a is not raw data, just keep it as 0
                        if (!a.isArray && a.raw !is null) {
                            // convert the data; supported lengths are 0, 1, 2, 4, 8
                            switch (a.raw.data.length) {
                                case 1:
                                    val = a.raw.data[0];
                                    break;
    
                                case 2:
                                    val =
                                        (a.raw.data[0] << 8) |
                                        (a.raw.data[1]);
                                    break;
    
                                case 4:
                                    val =
                                        (a.raw.data[0] << 24) |
                                        (a.raw.data[1] << 16) |
                                        (a.raw.data[2] << 8) |
                                        (a.raw.data[3]);
                                    break;
                                    
                                case 8:
                                    static if(ptrdiff_t.sizeof >= 8) {
                                        val =
                                            (a.raw.data[0] << 56) |
                                            (a.raw.data[1] << 48) |
                                            (a.raw.data[2] << 40) |
                                            (a.raw.data[3] << 32) |
                                            (a.raw.data[4] << 24) |
                                            (a.raw.data[5] << 16) |
                                            (a.raw.data[6] << 8) |
                                            (a.raw.data[7]);
                                    } else {
                                        val =
                                            (a.raw.data[4] << 24) |
                                            (a.raw.data[5] << 16) |
                                            (a.raw.data[6] << 8) |
                                            (a.raw.data[7]);
                                    }
                                    break;
    
                                default:
                                    throw new InterpreterFailure("Cannot create an integer from data of lengths other than 1, 2, 4, 8.");
                            }
                        }
    
                        // now put it in a raw data object
                        PSLRawData* rd = PSLRawData.allocate(
                            (cast(ubyte*) &val)[0..ptrdiff_t.sizeof]);
    
                        // and in a PSL object
                        PSLObject* no = PSLObject.allocate(context);
                        no.raw = rd;
    
                        // then push it
                        push(no);
                    });
                    break;
    
                case 0x71: // intwidth
                {
                    ptrdiff_t wid = ptrdiff_t.sizeof;
    
                    // put it in raw data
                    PSLRawData* rd = PSLRawData.allocate(
                        (cast(ubyte*) &wid)[0..ptrdiff_t.sizeof]);
    
                    // and an object
                    PSLObject* no = PSLObject.allocate(context);
                    no.raw = rd;
    
                    // then push it
                    push(no);
                    break;
                }
    
                case 0x72: // mul
                    intop((ptrdiff_t left, ptrdiff_t right) {
                        return left * right;
                    });
                    break;
    
                case 0x73: // div
                    intop((ptrdiff_t left, ptrdiff_t right) {
                        return left / right;
                    });
                    break;
    
                case 0x74: // mod
                    intop((ptrdiff_t left, ptrdiff_t right) {
                        return left % right;
                    });
                    break;
    
                case 0x76: // add
                    intop((ptrdiff_t left, ptrdiff_t right) {
                        return left + right;
                    });
                    break;
    
                case 0x77: // sub
                    intop((ptrdiff_t left, ptrdiff_t right) {
                        return left - right;
                    });
                    break;
    
                case 0x78:
                case 0x79:
                case 0x7A:
                case 0x7B:
                case 0x7C:
                case 0x7D: // integer comparisons
                {
                    PSLObject* thrown =
                        intcmp((ptrdiff_t left, ptrdiff_t right) {
                            bool res;
    
                            switch (cmd) {
                                case 0x78: // lt
                                    res = (left < right);
                                    break;
    
                                case 0x79: // lte
                                    res = (left <= right);
                                    break;
    
                                case 0x7A: // eq
                                    res = (left == right);
                                    break;
    
                                case 0x7B: // ne
                                    res = (left != right);
                                    break;
    
                                case 0x7C: // gt
                                    res = (left > right);
                                    break;
    
                                case 0x7D: // gte
                                    res = (left >= right);
                                    break;
                            }
    
                            return res;
                        });
    
                    if (thrown !is null) {
                        // threw something, so bail out
                        stack.truncate(lastlen);
                        return thrown;
                    }
                    break;
                }
    
                case 0x7E: // sl
                    intop((ptrdiff_t left, ptrdiff_t right) {
                        return left << right;
                    });
                    break;
    
                case 0x7F: // sr
                    intop((ptrdiff_t left, ptrdiff_t right) {
                        return left >> right;
                    });
                    break;
    
                case 0x80: // bitwise or
                    intop((ptrdiff_t left, ptrdiff_t right) {
                        return left | right;
                    });
                    break;
    
                case 0x81: // nor
                    intop((ptrdiff_t left, ptrdiff_t right) {
                        return ~(left | right);
                    });
                    break;
    
                case 0x82: // xor
                    intop((ptrdiff_t left, ptrdiff_t right) {
                        return left ^ right;
                    });
                    break;
    
                case 0x83: // nxor
                    intop((ptrdiff_t left, ptrdiff_t right) {
                        return ~(left ^ right);
                    });
                    break;
    
                case 0x84: // and
                    intop((ptrdiff_t left, ptrdiff_t right) {
                        return left & right;
                    });
                    break;
    
                case 0x85: // nand
                    intop((ptrdiff_t left, ptrdiff_t right) {
                        return ~(left & right);
                    });
                    break;
    
                case 0x8E: // byte
                    use1((PSLObject* a) {
                        if (!a.isArray && a.raw !is null &&
                            a.raw.data.length == ptrdiff_t.sizeof) {
                            // OK, everything is right, so get a single byte
                            ubyte sb = *(cast(ptrdiff_t*) a.raw.data.ptr) % 256;
    
                            // then put that in raw data
                            PSLRawData* rd = PSLRawData.allocate((&sb)[0..1]);
    
                            // and put that in an object
                            PSLObject* no = PSLObject.allocate(context);
                            no.raw = rd;
    
                            // and push it
                            push(no);
    
                        } else {
                            throw new InterpreterFailure("byte expects an integer operand.");
    
                        }
                    });
                    break;
    
                case 0x90: // float
                    use1((PSLObject* a) {
                        if (!a.isArray && a.raw !is null &&
                            a.raw.data.length == ptrdiff_t.sizeof) {
                            // convert it to a real
                            real res = *(cast(ptrdiff_t*) a.raw.data.ptr);
    
                            // put it in raw data
                            PSLRawData* rd = PSLRawData.allocate((cast(ubyte*) &res)[0..real.sizeof]);
    
                            // put it in an object
                            PSLObject* no = PSLObject.allocate(context);
                            no.raw = rd;
    
                            // and push it
                            push(no);
    
                        } else {
                            throw new InterpreterFailure("float expects an integer operand.");
    
                        }
                    });
                    break;
    
                case 0x91: // fint
                    use1((PSLObject* a) {
                        if (!a.isArray && a.raw !is null &&
                            a.raw.data.length == real.sizeof) {
                            // convert it to a ptrdiff_t
                            ptrdiff_t res = cast(ptrdiff_t)
                                *(cast(real*) a.raw.data.ptr);
    
                            // put it in raw data
                            PSLRawData* rd = PSLRawData.allocate(
                                (cast(ubyte*) &res)[0..ptrdiff_t.sizeof]);
    
                            // put it in an object
                            PSLObject* no = PSLObject.allocate(context);
                            no.raw = rd;
    
                            // and push it
                            push(no);
    
                        } else {
                            throw new InterpreterFailure("fint expects a floating-point operand.");
                        }
                    });
                    break;
    
                case 0x92: // fmul
                    floatop((real a, real b) {
                        return a * b;
                    });
                    break;
    
                case 0x93: // fdiv
                    floatop((real a, real b) {
                        return a / b;
                    });
                    break;
    
                case 0x94: // fmod
                    floatop((real a, real b) {
                        return a % b;
                    });
                    break;
    
                case 0x96: // fadd
                    floatop((real a, real b) {
                        return a + b;
                    });
                    break;
    
                case 0x97: // fsub
                    floatop((real a, real b) {
                        return a - b;
                    });
                    break;
    
                case 0x98:
                case 0x99:
                case 0x9A:
                case 0x9B:
                case 0x9C:
                case 0x9D: // float comparisons
                {
                    PSLObject* thrown =
                        floatcmp((real left, real right) {
                            bool res;
    
                            switch (cmd) {
                                case 0x98: // flt
                                    res = (left < right);
                                    break;
    
                                case 0x99: // lte
                                    res = (left <= right);
                                    break;
    
                                case 0x9A: // feq
                                    res = (left == right);
                                    break;
    
                                case 0x9B: // fne
                                    res = (left != right);
                                    break;
    
                                case 0x9C: // fgt
                                    res = (left > right);
                                    break;
    
                                case 0x9D: // fgte
                                    res = (left >= right);
                                    break;
                            }
    
                            return res;
                        });
    
                    if (thrown !is null) {
                        // threw something, so bail out
                        stack.truncate(lastlen);
                        return thrown;
                    }
                    break;
                }

                case 0xD0: // dsrcfile
                    use1((PSLObject* a) {
                        if (!a.isArray && a.raw !is null) {
                            if (dfile.length == 0) {
                                dfile = cast(char[]) a.raw.data.dup;
                            }
                        } else {
                            throw new InterpreterFailure("dsrcfile expects a raw data operand.");
                        }
                    });
                    break;

                case 0xD1: // dsrcline
                    use1((PSLObject* a) {
                        if (!a.isArray && a.raw !is null &&
                            a.raw.data.length == ptrdiff_t.sizeof) {
                            if (dline == -1)
                                dline = *(cast(ptrdiff_t*) a.raw.data.ptr);
                        } else {
                            throw new InterpreterFailure("dsrcline expects an integer operand.");
                        }
                    });
                    break;
    
                case 0xD2: // dsrccol
                    use1((PSLObject* a) {
                        if (!a.isArray && a.raw !is null &&
                            a.raw.data.length == ptrdiff_t.sizeof) {
                            if (dcol == -1)
                                dcol = *(cast(ptrdiff_t*) a.raw.data.ptr);
                        } else {
                            throw new InterpreterFailure("dsrccol expects an integer operand.");
                        }
                    });
                    break;

                case 0xE0: // FAKE: print
                    use1((PSLObject* a) {
                        if (!a.isArray && a.raw !is null) {
                            if (a.raw.data.length > 80) {
                                Stdout("Long string (procedure?)").newline;
                            } else {
                                Stdout(cast(char[]) a.raw.data).newline;
                            }
                            if (a.raw.data.length == ptrdiff_t.sizeof) {
                                Stdout(" = ")(*(cast(ptrdiff_t*) a.raw.data.ptr)).newline;
                            } else {
                                Stdout(" = ");
                                foreach (c; a.raw.data) {
                                    Stdout.format("{:X} ", c);
                                }
                                Stdout.newline;
                            }
                        } else {
                            if (a is pslNull) {
                                Stdout("NULL").newline;
                            } else if (a.isArray) {
                                Stdout("Array").newline;
                            } else {
                                Stdout(cast(void*) a).newline;
                            }
                        }
                    });
                    break;
    
                case 0xE1: // FAKE: debug
                    Stdout("STACK LENGTH: ")(stack.stack.length).newline;
                    break;
    
                case 0xEF: // parse
                    break;
    
                case 0xF0: // gadd
                    use3((PSLObject* a, PSLObject* b, PSLObject* c) {
                        if (!a.isArray && a.raw !is null &&
                            b.isArray &&
                            !c.isArray && c.raw !is null) {
                            // convert the array of raws into a ubyte[][]
                            ubyte[][] prod;
                            foreach (elem; b.arr.arr) {
                                if (!elem.isArray && elem.raw !is null) {
                                    prod ~= elem.raw.data.dup;
                                }
                            }
    
                            // then gadd them
                            if (prp !is null) {
                                prp.gadd(a.raw.data.dup, prod, c.raw.data.dup);
                            }
                        }
                    });
                    break;
    
                case 0xF1: // grem
                    use1((PSLObject* a) {
                        if (!a.isArray && a.raw !is null && prp !is null) {
                            prp.grem(a.raw.data.dup);
                        }
                    });
                    break;
    
                case 0xFD: // gcommit
                    if (prp !is null) {
                        prp.gcommit();
                    }
                    break;
    
                case 0xFE: // immediate, not yet implemented usefully
                    break;
    
                case 0xFF: // raw
                {
                    PSLRawData* rd = PSLRawData.allocate(sub);
                    PSLObject* no = PSLObject.allocate(context);
                    no.raw = rd;
                    push(no);
                    break;
                }
    
                default:
                    Stderr("Unrecognized operation ").format("{:X}", cmd).newline;
                    assert(0);
            }
    
            // Perhaps run the GC
            PlofGCable.maybeGC();
        }

    } catch (InterpreterFailure fail) {
        // something went wrong. Add our bit of error code if applicable
        if (dfile.length) {
            char[] err = Stdout.layout.convert("{}\n  in file {} line {} col {}",
                fail.msg,
                dfile, dline + 1, dcol + 1);
            throw new InterpreterFailure(err);
        } else {
            throw fail;
        }
    }

    return null;
}

/// Interpretation failures
class InterpreterFailure : Exception {
    this(char[] msg) { super(msg); }
}
