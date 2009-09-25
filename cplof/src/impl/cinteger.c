label(interp_psl_cinteger);
    DEBUG_CMD("cinteger");
    {
        ptrdiff_t val = 0;

        UNARY;

        /* get the value */
        if (ISRAW(a)) {
            rd = (struct PlofRawData *) a->data;
            val = parseRawCInt(rd);
        }

        PUSHINT(val);
    }
    STEP;
