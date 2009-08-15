#ifndef WITH_CNFI
label(interp_psl_ctype); UNIMPL("psl_ctype");
#else

label(interp_psl_ctype);
    DEBUG_CMD("ctype");
    UNARY;

    if (ISINT(a)) {
        int typenum = ASINT(a);
        ffi_type *type = NULL;

        switch (typenum) {
            case psl_ctype_void:
                type = &ffi_type_void;
                break;
            case psl_ctype_int:
                type = &ffi_type_sint;
                break;
            case psl_ctype_float:
                type = &ffi_type_float;
                break;
            case psl_ctype_double:
                type = &ffi_type_double;
                break;
            case psl_ctype_long_double:
                type = &ffi_type_longdouble;
                break;
            case psl_ctype_uint8:
                type = &ffi_type_uint8;
                break;
            case psl_ctype_int8:
                type = &ffi_type_sint8;
                break;
            case psl_ctype_uint16:
                type = &ffi_type_uint16;
                break;
            case psl_ctype_int16:
                type = &ffi_type_sint16;
                break;
            case psl_ctype_uint32:
                type = &ffi_type_uint32;
                break;
            case psl_ctype_int32:
                type = &ffi_type_sint32;
                break;
            case psl_ctype_uint64:
                type = &ffi_type_uint64;
                break;
            case psl_ctype_int64:
                type = &ffi_type_sint64;
                break;
            case psl_ctype_pointer:
                type = &ffi_type_pointer;
                break;
            case psl_ctype_uchar:
                type = &ffi_type_uchar;
                break;
            case psl_ctype_schar:
                type = &ffi_type_schar;
                break;
            case psl_ctype_ushort:
                type = &ffi_type_ushort;
                break;
            case psl_ctype_sshort:
                type = &ffi_type_sshort;
                break;
            case psl_ctype_uint:
                type = &ffi_type_uint;
                break;
            case psl_ctype_sint:
                type = &ffi_type_sint;
                break;
            case psl_ctype_ulong:
                type = &ffi_type_ulong;
                break;
            case psl_ctype_slong:
                type = &ffi_type_slong;
                break;
            case psl_ctype_ulonglong:
                type = &ffi_type_uint64;
                break;
            case psl_ctype_slonglong:
                type = &ffi_type_sint64;
                break;
            default:
                BADTYPE("ctype");
                type = &ffi_type_void;
        }

        PUSHPTR(type);

    } else {
        BADTYPE("ctype");
        STACK_PUSH(plofNull);

    }

    STEP;
#endif
