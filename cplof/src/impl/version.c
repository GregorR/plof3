label(interp_psl_version);
    DEBUG_CMD("version");
    {
        size_t i;
        struct PlofObject *otmp;
        ad = newPlofArrayData(VERSION_MAX);

#define CREATE_VERSION(str) \
        { \
            rd = newPlofRawData(strlen(str)); \
            memcpy(rd->data, str, rd->length); \
            \
            otmp = newPlofObject(); \
            otmp->parent = context; \
            otmp->data = (struct PlofData *) rd; \
            \
            ad->data[i++] = otmp; \
        }

        /* now go step by step */
        i = 0;
        CREATE_VERSION("cplof");

        /* CNFI, if applicable */
#ifdef WITH_CNFI
        CREATE_VERSION("CNFI");
#endif

        /* Standards */
#ifdef _POSIX_VERSION
        CREATE_VERSION("POSIX");
#endif

        /* Architectures */
#ifdef __arm__
        CREATE_VERSION("ARM");
#endif
#ifdef __mips__
        CREATE_VERSION("MIPS");
#endif
#ifdef __powerpc
        CREATE_VERSION("PowerPC");
#endif
#if defined(i386) || defined(__i386) || defined(_M_IX86) || \
    defined(__THW_INTEL__) || defined(__INTEL__) || defined(__amd64__)
        CREATE_VERSION("x86");
#endif
#ifdef __amd64__
        CREATE_VERSION("x86_64");
#endif

        /* Kernels */
#ifdef __APPLE__
        CREATE_VERSION("Darwin");
#endif
#ifdef MSDOS
        CREATE_VERSION("DOS");
#endif
#ifdef __FreeBSD__
        CREATE_VERSION("FreeBSD");
#endif
#ifdef __GNU__
        CREATE_VERSION("HURD");
#endif
#ifdef linux
        CREATE_VERSION("Linux");
#endif
#ifdef __NetBSD__
        CREATE_VERSION("NetBSD");
#endif
#ifdef __OpenBSD__
        CREATE_VERSION("OpenBSD");
#endif
#ifdef sun
        CREATE_VERSION("Solaris");
#endif

        /* Standard libraries */
#ifdef BSD
        CREATE_VERSION("BSD");
#endif
#ifdef __GLIBC__
        CREATE_VERSION("glibc");
#endif
#ifdef __APPLE__
        CREATE_VERSION("Mac OS X");
#endif
#ifdef _WIN32
        CREATE_VERSION("Windows");
#endif


        /* Finally, put it in an object */
        ad->length = i;
        otmp = newPlofObject();
        otmp->parent = context;
        otmp->data = (struct PlofData *) ad;
        STACK_PUSH(otmp);
    }
    STEP;
