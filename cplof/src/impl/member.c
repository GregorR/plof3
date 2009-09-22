label(interp_psl_member);
    DEBUG_CMD("member");
    BINARY;
    if (ISOBJ(a) && ISRAW(b)) {
        unsigned char *name;
        size_t namehash;
        struct PlofObject *otmp;
        rd = RAW(b);
        name = rd->data;
        HASHOF(namehash, rd);

        otmp = plofRead(a, rd->length, name, namehash);
        STACK_PUSH(otmp);
    } else {
        /*BADTYPE("member");*/
        STACK_PUSH(plofNull);
    }
    STEP;

