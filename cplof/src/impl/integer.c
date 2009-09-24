label(interp_psl_integer);
    DEBUG_CMD("integer");
    {
        ptrdiff_t val = 0;

        /* this is overloaded on whether the integer is already provided precomputed */
        if (pc[1]) {
#if defined(PLOF_BOX_NUMBERS)
            struct PlofObject *otmp;
            rd = (struct PlofRawData *) pc[1];
            otmp = newPlofObject();
            otmp->parent = context;
            otmp->data = (struct PlofData *) rd;
            STACK_PUSH(otmp);
#elif defined(PLOF_FREE_INTS)
            STACK_PUSH((struct PlofObject *) pc[1]);
#endif
        } else {
            UNARY;

            /* get the value */
            if (ISRAW(a)) {
                rd = (struct PlofRawData *) a->data;
                val = parseRawInt(rd);
            }
    
            PUSHINT(val);
        }
    }
    STEP;
