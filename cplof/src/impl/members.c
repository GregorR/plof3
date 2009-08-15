label(interp_psl_members);
    DEBUG_CMD("members");
    UNARY;
    if (ISOBJ(a)) {
        ad = plofMembers(a);
        b = GC_NEW_Z(struct PlofObject);
        b->parent = context;
        b->data = (struct PlofData *) ad;
        STACK_PUSH(b);
    } else {
        BADTYPE("members");
        STACK_PUSH(plofNull);
    }
    STEP;

