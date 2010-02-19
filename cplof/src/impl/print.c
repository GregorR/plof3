label(interp_psl_print);
    DEBUG_CMD("print");
    /* do our best to print this (debugging) */
    UNARY;
    if (ISRAW(a)) {
        fwrite(RAW(a)->data, 1, RAW(a)->length, stdout);
        fputc('\n', stdout);

#if defined(PLOF_BOX_NUMBERS)
        if (RAW(a)->length == sizeof(ptrdiff_t)) {
            printf("Integer value: %d\n", (int) *((ptrdiff_t *) RAW(a)->data));
        }
#endif
    } else if (ISOBJ(a)) {
        printf("%d %p\n", (int) ASINT(a), (void *) a);
    } else if (ISINT(a)) {
        printf("%ld\n", (long) ASINT(a));
    }
    STEP;

