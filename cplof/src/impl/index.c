label(interp_psl_index);
    DEBUG_CMD("index");
    BINARY;
    if (ISARRAY(a) && ISINT(b)) {
        ptrdiff_t index = ASINT(b);
        ad = ARRAY(a);

        if (index < 0 || index >= ad->length) {
            STACK_PUSH(plofNull);
        } else {
            STACK_PUSH(ad->data[index]);
        }
    } else {
        BADTYPE("index");
        STACK_PUSH(plofNull);
    }
    STEP;

