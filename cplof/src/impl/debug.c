label(interp_psl_debug);
    DEBUG_CMD("debug");
    {
        static unsigned int debugCount = 0;
        /*fprintf(stderr, "%u\r", ++debugCount);*/
        pid_t pid;
        char *corefn, *pidstr;

        debugCount++;
        corefn = GC_MALLOC_ATOMIC(32);
        snprintf(corefn, 16, "core.%u", debugCount);
        pidstr = GC_MALLOC_ATOMIC(32);
        snprintf(pidstr, 16, "%u", getpid());

        pid = fork();
        if (pid) {
            waitpid(pid, NULL, 0);
        } else {
            execlp("gcore", "gcore", "-o", corefn, pidstr, NULL);
        }
    }
    STEP;

