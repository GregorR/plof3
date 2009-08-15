label(interp_psl_member);
    DEBUG_CMD("member");
    BINARY;
    if (ISOBJ(a) && ISRAW(b)) {
        unsigned char *name;
        size_t namehash;
        rd = RAW(b);
        name = rd->data;
        HASHOF(namehash, rd);

        a = plofRead(a, rd->length, name, namehash);
        STACK_PUSH(a);
    } else {
        /*BADTYPE("member");*/
        STACK_PUSH(plofNull);
    }
    STEP;

