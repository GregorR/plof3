#ifndef WITH_CNFI
label(interp_psl_csizeof); UNIMPL("psl_csizeof");
#else

label(interp_psl_csizeof);
    DEBUG_CMD("csizeof");
    UNARY;
    if (ISPTR(a)) {
        /* this had better be an ffi_type pointer :) */
        ffi_type *type = (ffi_type *) ASPTR(a);
        PUSHINT(type->size);
    } else {
        BADTYPE("csizeof");
        STACK_PUSH(plofNull);
    }
    STEP;
#endif
