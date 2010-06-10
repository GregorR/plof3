ARITY(5)
PUSHES(1)
LEAKA
LEAKP

#ifdef PSL_OPTIM
if (cpsli >= 4 && cpsl[cpsli-4] == pslCompileLabels[label_psl_code] && cpsl[cpsli-2] == pslCompileLabels[label_psl_code]) {
    struct PlofRawData *rda, *rdb;
    rda = (struct PlofRawData *) cpslargs[cpslai-2];
    rdb = (struct PlofRawData *) cpslargs[cpslai-1];
    if (pslCanInline(rda->length, rda->data) &&
        pslCanInline(rda->length, rda->data)) {
        void **cpsla, **cpslb;
        size_t cpsllena, cpsllenb, ssa, ssb;
        size_t cpslibase;
        cpsli -= 4;
        cpslibase = cpsli;

        /* compile the first one */
        ret = compilePSL(rda->length, rda->data, 0, &cpslalen, &cpslai, cpslargs, &cpsllena, &cpslargs);
        cpsla = ((struct CPSLArgsHeader *) cpslargs)->cpsl;
        if (stacksize + ((struct CPSLArgsHeader *) cpslargs)->maxstacksize > maxstacksize)
            maxstacksize = stacksize + ((struct CPSLArgsHeader *) cpslargs)->maxstacksize;
        ssa = ((struct CPSLArgsHeader *) cpslargs)->endstacksize;
        if (ret.isThrown) return ret;

        /* compile the second one */
        ret = compilePSL(rdb->length, rdb->data, 0, &cpslalen, &cpslai, cpslargs, &cpsllenb, &cpslargs);
        cpslb = ((struct CPSLArgsHeader *) cpslargs)->cpsl;
        if (stacksize + ((struct CPSLArgsHeader *) cpslargs)->maxstacksize > maxstacksize)
            maxstacksize = stacksize + ((struct CPSLArgsHeader *) cpslargs)->maxstacksize;
        ssb = ((struct CPSLArgsHeader *) cpslargs)->endstacksize;
        if (ret.isThrown) return ret;

        /* put in the first jump instruction */
        cpsl[cpsli] = pslCompileLabels[label_psl_jne];
        cpsl[cpsli+1] = (void *) (cpsllena + 2); /* + 2 for the intervening jmp */
        cpsli += 2;

        /* now append the first bit */
        while (cpsllen < cpsli + cpsllena + 2) {
            cpsllen *= 2;
            cpsl = GC_REALLOC(cpsl, cpsllen * sizeof(void *));
        }
        memcpy(cpsl + cpsli, cpsla, cpsllena * sizeof(void *));
        cpsli += cpsllena - 2;

        /* replace the ending return with a stackfrunge */
        cpsl[cpsli] = pslCompileLabels[label_psl_stackfrunge];
        cpsl[cpsli+1] = (void *) ssa;
        cpsli += 2;

        /* and add the jmp */
        cpsl[cpsli] = pslCompileLabels[label_psl_jmp];
        cpsl[cpsli+1] = (void *) cpsllenb;
        cpsli += 2;

        /* append the second bit */
        while (cpsllen < cpsli + cpsllenb) {
            cpsllen *= 2;
            cpsl = GC_REALLOC(cpsl, cpsllen * sizeof(void *));
        }
        memcpy(cpsl + cpsli, cpslb, cpsllenb * sizeof(void *));
        cpsli += cpsllenb - 2;

        /* replace the ending return with a stackfrunge */
        cpsl[cpsli] = pslCompileLabels[label_psl_stackfrunge];
        cpsl[cpsli+1] = (void *) ssb;

        /* finally, we now leak everything */
        LEAKALL;
    }
}
#endif
