/**
 * A lightweight system for class extensions in D.
 * 
 * Copyright (C) Gregor Richards, 2008
 * 
 * License:
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *  
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *  
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *  THE SOFTWARE.
 */

/* Use:
 * To make a function int foo(int a, int b) which extends the class A:
 * mixin(extension("A", "int", "foo", "int a, int b", "a, b"));
 * 
 * Then to declare a function as an extension for A or a subclass:
 * int A_foo(A pthis, int a, int b) { ... }
 * mixin(extend("A", "foo"));
 * 
 * To call the function, use the object as the first argument:
 * return foo(a, 1, 2);
 */

module extensions;

/// Maximum extension number currently
int extensionCount = 0;

/// Basic generator for extensions
char[] extension(char[] cl, char[] rtype, char[] fun, char[] args, char[] sargs) {
    if (args != "") args = ", " ~ args;
    if (sargs != "") sargs = ", " ~ sargs;

    char[] ret = `void*[ClassInfo] __ext_` ~ fun ~ `;
    ` ~ rtype ~ ` ` ~ fun ~ `(` ~ cl ~ ` pthis` ~ args ~ `) {
        // go through super-classes until we lose or find the extension
        ClassInfo ci = pthis.classinfo;
        while (ci !is null) {
            void** ext = ci in __ext_` ~ fun ~ `;
            if (ext !is null) {
                void* f = *ext;

                /* found it! */`;

                if (rtype != "void") ret ~= "return ";

                ret ~= `(cast(` ~ rtype ~ ` function(` ~ cl ~ args ~ `)) f)(pthis` ~ sargs ~ `);`;

                if (rtype == "void") ret ~= "return;";

                ret ~= `
            }
            ci = ci.base;
        }
        throw new ExtensionLookupFailure();
    }`;

    return ret;
}

/// To make a specific extension
char[] extend(char[] cl, char[] fun) {
    return `class __ext_` ~ cl ~ `_` ~ fun ~ `_staticthis {
        static this() {
            __ext_` ~ fun ~ `[` ~ cl ~ `.classinfo] = cast(void*) &` ~ cl ~ `_` ~ fun ~ `;
        }
    }`;
}

class ExtensionLookupFailure : Exception {
    this() {
        super("ExtensionLookupFailure");
    }
}
