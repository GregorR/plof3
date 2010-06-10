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

#endif
