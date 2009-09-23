arity = 1;
pushes = 1;

/* if the value is already known, translate it in advance */
if (cpsli >= 2 && cpsl[cpsli-2] == addressof(interp_psl_raw)) {
    /* mark the input as leaked so it isn't improperly deleted */
    leaka = 1;

    /* now replace the integer */
    cpsl[cpsli-2] = cpsl[cpsli];
    cpsli -= 2;
    RDINT(parseRawInt((struct PlofRawData *) cpsl[cpsli+1]));
    cpsl[cpsli+1] = rd;
}
