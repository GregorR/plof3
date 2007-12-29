/**
 * A simple tree map implementation using PSL's GC
 *
 * License:
 *  Copyright (c) 2007  Gregor Richards
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

module plof.psl.treemap;

import tango.text.Util;

import tango.stdc.stdlib;

import plof.psl.gc;

/// A treemap, mapping char[]'s to PlofGCable's.
struct PlofTreemap {
    PlofGCable gc;

    /// Allocate a treemap
    static PlofTreemap* allocate()
    {
        PlofTreemap* ret = PlofGCable.allocate!(PlofTreemap)();
        ret.gc.foreachRef = &ret.foreachRef;
        return ret;
    }

    /// Foreach over elements
    void foreachRef(void delegate(PlofGCable*) callback)
    {
        if (node !is null) {
            callback(cast(PlofGCable*) node);
        }
    }

    // Foreach over values
    void foreachVal(void delegate(char[] name, PlofGCable*) callback)
    {
        if (node !is null) {
            node.foreachVal(callback);
        }
    }

    /// Add an element
    void add(char[] name, PlofGCable* val)
    {
        if (node is null) {
            node = PlofTreemapNode.allocate(name, val);
            node.gc.refUp();
        } else {
            node.add(name, val);
        }
    }

    /// Get an element
    PlofGCable* get(char[] name)
    {
        if (node is null)
            return null;
        return node.get(name);
    }

    /// The node which is the top of this treemap
    PlofTreemapNode* node;
}

/// A treemap node
struct PlofTreemapNode {
    PlofGCable gc;

    /// Allocate a treemap node with the given name and value
    static PlofTreemapNode* allocate(char[] name, PlofGCable* val)
    {
        PlofTreemapNode* ret = PlofGCable.allocate!(PlofTreemapNode)();
        ret.gc.foreachRef = &ret.foreachRef;
        ret.gc.destructor = &ret.destructor;

        // duplicate the name with C allocation to avoid D's GC
        ret.name = (cast(char*) malloc(name.length))[0..name.length];
        ret.name[] = name[];

        ret.hash = jhash(cast(ubyte*) name.ptr, name.length);
        
        ret.val = val;
        val.refUp();
        return ret;
    }

    /// Foreach over references
    void foreachRef(void delegate(PlofGCable*) callback)
    {
        callback(cast(PlofGCable*) val);

        if (left !is null) {
            callback(cast(PlofGCable*) left);
        }
        if (right !is null) {
            callback(cast(PlofGCable*) right);
        }
    }

    /// Standard foreach
    void foreachVal(void delegate(char[], PlofGCable*) callback)
    {
        if (left !is null) {
            left.foreachVal(callback);
        }
        callback(name, val);
        if (right !is null) {
            right.foreachVal(callback);
        }
    }

    void destructor()
    {
        free(name.ptr);
    }

    /// Add an element under this treemap node
    void add(char[] name, PlofGCable* val)
    {
        add(name, jhash(cast(ubyte*) name.ptr, name.length), val);
    }

    /// Add an element under this treemap node with the given hash
    void add(char[] name, uint hash, PlofGCable* val)
    {
        if (hash < this.hash) {
            if (left is null) {
                left = PlofTreemapNode.allocate(name, val);
                left.gc.refUp();
            } else {
                left.add(name, hash, val);
            }

        } else if (hash == this.hash) {
            // replace
            PlofGCable* lastval = this.val;
            this.val = val;
            val.refUp();
            lastval.refDown();

        } else {
            if (right is null) {
                right = PlofTreemapNode.allocate(name, val);
                right.gc.refUp();
            } else {
                right.add(name, hash, val);
            }
        }
    }

    /// Get an element under this treemap node
    PlofGCable* get(char[] name)
    {
        return get(jhash(cast(ubyte*) name.ptr, name.length));
    }

    /// Get an element under this treemap node with the given hash
    PlofGCable* get(uint hash)
    {
        if (hash < this.hash) {
            if (left is null) {
                return null;
            } else {
                return left.get(hash);
            }

        } else if (hash == this.hash) {
            // this is the one
            return val;

        } else {
            if (right is null) {
                return null;
            } else {
                return right.get(hash);
            }
        }
    }

    // The actual mapping
    char[] name;
    uint hash;
    PlofGCable* val;

    // And child nodes
    PlofTreemapNode* left, right;
}
