label(interp_psl_include);
#define BUFSZ 1024
    DEBUG_CMD("include");
    UNARY;

    if (ISRAW(a)) {
        unsigned char **path, *file, *data;
        FILE *fh;
        size_t rdb, bufsz, bufi;
        struct PlofObject *otmp;

        rd = RAW(a);

        /* look for the file in each path */
        data = NULL;
        for (path = plofIncludePaths; *path; path++) {
            file = GC_MALLOC_ATOMIC(strlen((char *) *path) + rd->length + 2);
            sprintf((char *) file, "%s/%.*s", (char *) *path, (int) rd->length, (char *) rd->data);

            fh = fopen((char *) file, "r");
            if (fh != NULL) {
                /* this file exists, use it */
                bufsz = BUFSZ;
                bufi = 0;
                data = malloc(BUFSZ);

                while ((rdb = fread(data + bufi, 1, bufsz - bufi, fh)) > 0) {
                    bufi += rdb;
                    if (bufi == bufsz) {
                        bufsz *= 2;
                        data = realloc(data, bufsz);
                    }
                }

                fclose(fh);

                break;
            }
        }

        /* if we didn't find it, push NULL, otherwise push the rd */
        if (data == NULL) {
            STACK_PUSH(plofNull);

        } else {
            rd = newPlofRawData(bufi);
            memcpy(rd->data, data, bufi);
            free(data);

            otmp = newPlofObject();
            otmp->parent = context;
            otmp->data = (struct PlofData *) rd;

            STACK_PUSH(otmp);
        }

    } else {
        BADTYPE("include");
        STACK_PUSH(plofNull);

    }
    STEP;

