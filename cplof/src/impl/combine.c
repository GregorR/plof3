label(interp_psl_combine);
    DEBUG_CMD("combine");
    BINARY;

    if (!ISOBJ(a) || !ISOBJ(b)) {
        /* FIXME */
        BADTYPE("combine");
        STACK_PUSH(plofNull);
        STEP;
    }

    {
        struct PlofObject *newo;

        /* start making the new object */
        newo = newPlofObject();
        newo->parent = b->parent;
    
        /* duplicate the left object */
        plofObjCopy(newo, a);
    
        /* then the right */
        plofObjCopy(newo, b);
    
        /* now get any data */
        if (ISRAW(a)) {
            if (ISRAW(b)) {
                struct PlofRawData *ra, *rb;
                ra = RAW(a);
                rb = RAW(b);
    
                rd = newPlofRawData(ra->length + rb->length);
    
                /* copy in both */
                memcpy(rd->data, ra->data, ra->length);
                memcpy(rd->data + ra->length, rb->data, rb->length);
    
                newo->data = (struct PlofData *) rd;
    
            } else {
                /* just the left */
                struct PlofRawData *ra = (struct PlofRawData *) a->data;
                rd = newPlofRawData(ra->length);
                memcpy(rd->data, ra->data, rd->length);
                newo->data = (struct PlofData *) rd;
    
            }
    
        } else if (ISARRAY(a)) {
            if (ISRAW(b)) {
                /* just the right */
                struct PlofRawData *rb = (struct PlofRawData *) b->data;
                rd = newPlofRawData(rb->length);
                memcpy(rd->data, rb->data, rd->length);
                newo->data = (struct PlofData *) rd;
    
            } else if (ISARRAY(b)) {
                /* combine the arrays */
                struct PlofArrayData *aa, *ab;
                aa = ARRAY(a);
                ab = ARRAY(b);
    
                ad = newPlofArrayData(aa->length + ab->length);
    
                /* copy in both */
                memcpy(ad->data, aa->data, aa->length * sizeof(struct PlofObject *));
                memcpy(ad->data + aa->length, ab->data, ab->length * sizeof(struct PlofObject *));
    
                newo->data = (struct PlofData *) ad;
    
            } else {
                /* duplicate the left array */
                struct PlofArrayData *aa = ARRAY(a);
                ad = newPlofArrayData(aa->length);
                memcpy(ad->data, aa->data, ad->length * sizeof(struct PlofObject *));
    
            }
    
        } else {
            if (ISRAW(b)) {
                struct PlofRawData *rb = (struct PlofRawData *) b->data;
                rd = newPlofRawData(rb->length);
                memcpy(rd->data, rb->data, rd->length);
                newo->data = (struct PlofData *) rd;
    
            } else if (ISARRAY(b)) {
                /* duplicate the right array */
                struct PlofArrayData *ab = ARRAY(b);
                ad = newPlofArrayData(ab->length);
                ad->type = PLOF_DATA_ARRAY;
                memcpy(ad->data, ab->data, ad->length * sizeof(struct PlofObject *));
    
            }
    
        }
    
        STACK_PUSH(newo);
    }

    STEP;

