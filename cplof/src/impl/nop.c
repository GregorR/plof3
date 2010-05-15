label(interp_psl_nop);
    DEBUG_CMD("nop");
    /* do nothing */
    STEP;

    /* General "function" for PSL push* commands */
#define PSL_PUSH(n) \
    DEBUG_CMD("push"); \
    if (stacktop <= n) { \
        STACK_PUSH(plofNull); \
    } else { \
        STACK_PUSH(stack[stacktop - n - 1]); \
    } \
    STEP
