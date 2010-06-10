ARITY(0)
PUSHES(1)
LEAKP
LEAKALL

#ifdef PSL_OPTIM
/* get the arity manually */
if (cpsli >= 2 && cpsl[cpsli-2] == pslCompileLabels[label_psl_integer] && cpsl[cpsli-1] != NULL) {
    /* good, this is an integer literal, as it must be */
    cpsl[cpsli-2] = cpsl[cpsli];
    cpsli -= 2;

    /* now we can get the arity */
#if defined(PLOF_BOX_NUMBERS)
    rd = (struct PlofRawData *) cpslargs[(int) (size_t) cpsl[cpsli+1]];
    arity = *((ptrdiff_t *) rd->data);
#elif defined(PLOF_FREE_INTS)
    arity = ASINT(cpslargs[(int) (size_t) cpsl[cpsli+1]]);
#endif

} else {
    rd = newPlofRawData(128);
    sprintf((char *) rd->data, "'array' instruction with nonstatic size.");
    a = newPlofObject();
    a->parent = plofNull;
    a->data = (struct PlofData *) rd;

    ret.ret = newPlofObject();
    ret.ret->parent = plofNull;
    plofWrite(ret.ret, (unsigned char *) PSL_EXCEPTION_STACK, plofHash(sizeof(PSL_EXCEPTION_STACK)-1, (unsigned char *) PSL_EXCEPTION_STACK), a);
    ret.isThrown = 1;
    return ret;
}
#endif
