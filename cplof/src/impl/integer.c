label(interp_psl_integer);
    DEBUG_CMD("integer");
    UNARY;
    {
        ptrdiff_t val = 0;

        /* get the value */
        if (ISRAW(a)) {
            rd = (struct PlofRawData *) a->data;

            switch (rd->length) {
                case 1:
                    val = rd->data[0];
                    break;

                case 2:
                    val = ((ptrdiff_t) rd->data[0] << 8) |
                          ((ptrdiff_t) rd->data[1]);
                    break;

                case 4:
#if SIZEOF_VOID_P < 8
                case 8:
#endif
                    val = ((ptrdiff_t) rd->data[0] << 24) |
                          ((ptrdiff_t) rd->data[1] << 16) |
                          ((ptrdiff_t) rd->data[2] << 8) |
                          ((ptrdiff_t) rd->data[3]);
                    break;

#if SIZEOF_VOID_P >= 8
                case 8:
                    val = ((ptrdiff_t) rd->data[0] << 56) |
                          ((ptrdiff_t) rd->data[1] << 48) |
                          ((ptrdiff_t) rd->data[2] << 40) |
                          ((ptrdiff_t) rd->data[3] << 32) |
                          ((ptrdiff_t) rd->data[4] << 24) |
                          ((ptrdiff_t) rd->data[5] << 16) |
                          ((ptrdiff_t) rd->data[6] << 8) |
                          ((ptrdiff_t) rd->data[7]);
                    break;
#endif
            }
        }

        PUSHINT(val);
    }
    STEP;
