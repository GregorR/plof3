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

/// A treemap, mapping char[]'s to void*'s.
class PlofTreemap {
    // Foreach over values
    void foreachVal(void delegate(char[] name, void*) callback)
    {
        if (node !is null) {
            node.foreachVal(callback);
        }
    }

    /// Add an element
    void add(char[] name, void* val)
    {
        if (node is null) {
            node = new PlofTreemapNode(name, val);
        } else {
            node.add(name, val);
        }
    }

    /// Get an element
    void* get(char[] name)
    {
        if (node is null)
            return null;
        return node.get(name);
    }

    /// The node which is the top of this treemap
    PlofTreemapNode node;
}

/// A treemap node
class PlofTreemapNode {
    /// Allocate a treemap node with the given name and value
    this(char[] name, void* val)
    {
        this.name = new char[name.length];
        this.name[] = name[];

        this.hash = jhash(cast(ubyte*) name.ptr, name.length);
        
        this.val = val;
    }

    /// Standard foreach
    void foreachVal(void delegate(char[], void*) callback)
    {
        if (left !is null) {
            left.foreachVal(callback);
        }
        callback(name, val);
        if (right !is null) {
            right.foreachVal(callback);
        }
    }

    /// Add an element under this treemap node
    void add(char[] name, void* val)
    {
        add(name, jhash(cast(ubyte*) name.ptr, name.length), val);
    }

    /// Add an element under this treemap node with the given hash
    void add(char[] name, uint hash, void* val)
    {
        if (hash < this.hash) {
            if (left is null) {
                left = new PlofTreemapNode(name, val);
            } else {
                left.add(name, hash, val);
            }

        } else if (hash == this.hash) {
            // replace
            this.val = val;

        } else {
            if (right is null) {
                right = new PlofTreemapNode(name, val);
            } else {
                right.add(name, hash, val);
            }
        }
    }

    /// Get an element under this treemap node
    void* get(char[] name)
    {
        return get(jhash(cast(ubyte*) name.ptr, name.length));
    }

    /// Get an element under this treemap node with the given hash
    void* get(uint hash)
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
    void* val;

    // And child nodes
    PlofTreemapNode left, right;
}
