#ifndef INTERP_H
#define INTERP_H

/* We require an enum for compilation */                
enum PSLCompileLabel {                                     
#define FOREACH(inst) label_psl_ ## inst,               
#include "psl_internal_inst.h"                          
#undef FOREACH                                          
                                                        
    label_psl_last                                      
};                                                      
extern void *pslCompileLabels[label_psl_last];             

/* The extra data held at the beginning of cpslargs */
struct CPSLArgsHeader {
    void **cpsl;
    size_t cpsllen;
    size_t maxstacksize;
    size_t endstacksize;
};
#define CPSL_ARGS_HEADER_LENGTH 4

#endif
