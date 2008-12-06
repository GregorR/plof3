/**
 * Automatically-parallelizing interpreter for the PSL AST
 */

module plof.ap.interp;

import tango.io.Stdout;

import extensions;

import plof.ast.ast;

mixin(extension("PASTNode", "void", "astVisit", "", ""));


mixin(extend("PASTNode", "astVisit"));
void PASTNode_astVisit(PASTNode pthis) {
    Stdout("Unimplemented AST visitor for ")(pthis.classinfo.name).newline;
}


mixin(extend("PASTProc", "astVisit"));
void PASTProc_astVisit(PASTProc pthis) {
    Stdout("Proc {").newline;
    foreach (n; pthis.stmts) {
        astVisit(n);
    }
    Stdout("}").newline;
}
