ARITY(1)
PUSHES(1)

#ifdef PSL_OPTIM
/* if the value is already known, translate it in advance */
if (cpsli >= 2 && cpsl[cpsli-2] == pslCompileLabels[label_psl_raw]) {
    /* mark the input as leaked so it isn't improperly deleted */
    LEAKA

    /* now replace the integer */
    cpsl[cpsli-2] = cpsl[cpsli];
    cpsli -= 2;
    RDINT(parseRawInt((struct PlofRawData *) cpslargs[(int) (size_t) cpsl[cpsli+1]]));
    cpslargs[(int) (size_t) cpsl[cpsli+1]] = rd;
}
#endif
