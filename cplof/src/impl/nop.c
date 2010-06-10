label(interp_psl_nop);
    DEBUG_CMD("nop");
    /* do nothing */
    STEP;

    /* General "function" for PSL push* commands */
#define PSL_PUSH(n) \
    DEBUG_CMD("push"); \
    STACK_PUSH(*(stacktop - n - 1)); \
    STEP
