label(interp_psl_member);
    DEBUG_CMD("member");
    BINARY;
    if (ISOBJ(a) && ISRAW(b)) {
        unsigned char *name;
        struct PlofObject *otmp;
        rd = RAW(b);
        name = rd->data;

        otmp = plofRead(a, name);
        STACK_PUSH(otmp);
    } else {
        /*BADTYPE("member");*/
        STACK_PUSH(plofNull);
    }
    STEP;

