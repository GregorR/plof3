label(interp_psl_dinstruction_sequence_start);
    DEBUG_CMD("dinstruction_sequence_start");
#ifdef DEBUG_INSTRUCTION_SEQUENCE
    if (!debug_instruction_sequence_fh) {
        SF(debug_instruction_sequence_fh, fopen, NULL, ("plof_instruction_sequence.txt", "a"));
    }
#endif
    STEP;

