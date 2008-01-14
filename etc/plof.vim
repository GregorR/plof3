syn keyword     pulKeyword      as forceEval is in parent return var
syn keyword     pslKeyword      contained push0 push1 push2 push3 push4 push5 push6 push7 pop this null global new combine member memberset parent parentset call return throw catch cmp concat wrap iwrap loop array aconcat length lengthset index indexset memberset integer intwidth mul div mod add sub lt lte eq ne gt gte sl sr or nor xor nxor and nand byte float fint fmul fdiv fmod fadd fsub flt flte feq fne fgt fgte print debug parse gadd grem gaddstop gaddgroup gremgroup gcommit

syn region      plofLineComment start=+//+ end=+$+
syn region      plofMLComment   start=+/\*+ end=+\*/+

syn region      plofString      start=+"+ skip=+\\"+ end=+"+

syn region      pslBlock        contained start=+{+ end=+}+ contains=pslKeyword,plofNumber,plofLineComment,plofMLComment,plofString,pslBlock fold
syn region      plofPsl         start=+psl[ \t\r\n]*{+ end=+}+ contains=pslKeyword,plofNumber,plofLineComment,plofMLComment,plofString,pslBlock fold

syn region      plofFunction    start=+{+ end=+}+ contains=pulKeyword,plofNumber,plofLineComment,plofMLComment,plofString,plofPsl fold

syn region      plofObject      start=+:[ \t\r\n]*\[+ end=+\]+ contains=pulKeyword,plofNumber,plofLineComment,plofMLComment,plofString,plofFunction fold

hi def link pulKeyword          Keyword
hi def link pslKeyword          Keyword
hi def link plofLineComment     Comment
hi def link plofMLComment       Comment
hi def link plofString          String

let b:current_syntax = "plof"
