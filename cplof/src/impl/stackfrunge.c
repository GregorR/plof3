label(interp_psl_stackfrunge);
    DEBUG_CMD("stackfrunge");
    /* turn our n elements on top of the stack into just one */
    {
        size_t n = ((size_t) pc[1]) - 1;
        UNARY;
        stacktop -= n;
        STACK_PUSH(a);
    }
    STEP;

