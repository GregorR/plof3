syn keyword     pulKeyword      as by forceEval is in include parent return rtInclude to var
syn keyword     pslKeyword      contained push0 push1 push2 push3 push4 push5 push6 push7 pop this null global new combine member memberset parent parentset call return throw catch cmp concat wrap resolve while calli replace array aconcat length lengthset index indexset members rawlength slice rawcmp extractraw integer intwidth mul div mod add sub lt lte eq ne gt gte sl sr or nor xor nxor and nand byte float fint fmul fdiv fmod fadd fsub flt flte feq fne fgt fgte version dsrcfile dsrcline dsrccol print debug intrinsic trap include parse gadd grem gcommit marker immediate code raw dlopen dlclose dlsym cget cset cinteger ctype cstruct csizeof csget csset prepcif ccall

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
