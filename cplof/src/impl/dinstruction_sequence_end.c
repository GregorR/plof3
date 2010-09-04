label(interp_psl_dinstruction_sequence_end);
    DEBUG_CMD("dinstruction_sequence_end");
#ifdef DEBUG_INSTRUCTION_SEQUENCE
    if (debug_instruction_sequence_fh) {
        fclose(debug_instruction_sequence_fh);
        debug_instruction_sequence_fh = NULL;
    }
#endif
    STEP;

