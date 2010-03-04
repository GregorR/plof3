label(interp_psl_immediate);
    DEBUG_CMD("immediate");
    if (immediate) {
        rd = (struct PlofRawData *) cpslargs[(int) (size_t) pc[1]];
#ifdef DEBUG
        fprintf(stderr, "DEBUG: ");
        {
            int i;
            for (i = 0; i < rd->length; i++) {
                fprintf(stderr, "%02X", rd->data[i]);
            }
        }
        fprintf(stderr, "\n");
#endif
        interpretPSL(context, plofNull, NULL, rd->length, rd->data, 0, 0);
    }
    STEP;

