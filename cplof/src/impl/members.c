label(interp_psl_members);
    DEBUG_CMD("members");
    UNARY;
    if (ISOBJ(a)) {
        struct PlofObject *otmp;
        ad = plofMembers(a);
        otmp = newPlofObject();
        otmp->parent = context;
        otmp->data = (struct PlofData *) ad;
        STACK_PUSH(otmp);
    } else {
        BADTYPE("members");
        STACK_PUSH(plofNull);
    }
    STEP;

