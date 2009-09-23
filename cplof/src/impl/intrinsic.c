label(interp_psl_intrinsic);
    DEBUG_CMD("intrinsic");
    BINARY;
    if (ISRAW(a) && ISINT(b)) {
        /* save the intrinsic function represented by b into a, push a */
        ptrdiff_t intr = ASINT(b);
        rd = RAW(a);
        rd->proc = plofIntrinsics[intr];
        STACK_PUSH(a);
    } else {
        BADTYPE("intrinsic");
        STACK_PUSH(plofNull);
    }
    STEP;
