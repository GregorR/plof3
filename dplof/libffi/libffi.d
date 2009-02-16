/**
 * D version of the libffi headers.
 * As a simple translation (and therefore derivative work), this is under the
 * same license as libffi itself:
 *
 * License:
 *  Permission is hereby granted, free of charge, to any person obtaining
 *  a copy of this software and associated documentation files (the
 *  ``Software''), to deal in the Software without restriction, including
 *  without limitation the rights to use, copy, modify, merge, publish,
 *  distribute, sublicense, and/or sell copies of the Software, and to
 *  permit persons to whom the Software is furnished to do so, subject to
 *  the following conditions:

 *  The above copyright notice and this permission notice shall be included
 *  in all copies or substantial portions of the Software.

 *  THE SOFTWARE IS PROVIDED ``AS IS'', WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *  IN NO EVENT SHALL CYGNUS SOLUTIONS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *  OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 *  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *  OTHER DEALINGS IN THE SOFTWARE.
 */

module libffi.libffi;

version (build) {
    pragma(link, "ffi");
}

extern (C) {

struct ffi_type
{
  size_t size;
  ushort alignment;
  ushort type;
  ffi_type **elements;
};

extern ffi_type ffi_type_void;
extern ffi_type ffi_type_uint8;
extern ffi_type ffi_type_sint8;
extern ffi_type ffi_type_uint16;
extern ffi_type ffi_type_sint16;
extern ffi_type ffi_type_uint32;
extern ffi_type ffi_type_sint32;
extern ffi_type ffi_type_uint64;
extern ffi_type ffi_type_sint64;
extern ffi_type ffi_type_float;
extern ffi_type ffi_type_double;
extern ffi_type ffi_type_pointer;
extern ffi_type ffi_type_longdouble;

enum ffi_status {
  FFI_OK = 0,
  FFI_BAD_TYPEDEF,
  FFI_BAD_ABI
}

alias uint FFI_TYPE;

version (Alpha) {
    enum ffi_abi {
        FFI_FIRST_ABI = 0,
        FFI_OSF,
        FFI_DEFAULT_ABI = ffi_abi.FFI_OSF,
        FFI_LAST_ABI = ffi_abi.FFI_DEFAULT_ABI + 1
    }

} else version (ARM) {
    enum ffi_abi {
        FFI_FIRST_ABI = 0,
        FFI_SYSV,
        FFI_DEFAULT_ABI = FFI_SYSV,
    }

} else version (X86_64) {
    enum ffi_abi {
        FFI_FIRST_ABI = 0,
        FFI_SYSV,
        FFI_UNIX64,
        FFI_DEFAULT_ABI = FFI_UNIX64
    }

} else version (X86) {
    version (Windows) {
        enum ffi_abi {
            FFI_FIRST_ABI = 0,
            FFI_SYSV,
            FFI_STDCALL,
            FFI_DEFAULT_ABI = FFI_SYSV,
        }

    } else {
        enum ffi_abi {
            FFI_FIRST_ABI = 0,
            FFI_SYSV,
            FFI_UNIX64,
            FFI_DEFAULT_ABI = FFI_SYSV
        }

    }

} else version (MIPS) {
    enum ffi_abi {
        FFI_FIRST_ABI = 0,
        FFI_O32,
        FFI_N32,
        FFI_N64,
        FFI_O32_SOFT_FLOAT,
        FFI_DEFAULT_ABI = FFI_O32
    }

} else version (PPC64) {
    version (linux) {
        enum ffi_abi {
            FFI_FIRST_ABI = 0,
            FFI_SYSV,
            FFI_GCC_SYSV,
            FFI_LINUX64,
            FFI_LINUX,
            FFI_DEFAULT_ABI = FFI_LINUX64
        }

    } else version (aix) {
        enum ffi_abi {
            FFI_FIRST_ABI = 0,
            FFI_AIX,
            FFI_DARWIN,
            FFI_DEFAULT_ABI = FFI_AIX
        }

    } else version (darwin) {
        enum ffi_abi {
            FFI_FIRST_ABI = 0,
            FFI_AIX,
            FFI_DARWIN,
            FFI_DEFAULT_ABI = FFI_DARWIN
        }

    } else version (freebsd) {
        enum ffi_abi {
            FFI_FIRST_ABI = 0,
            FFI_SYSV,
            FFI_GCC_SYSV,
            FFI_LINUX64,
            FFI_DEFAULT_ABI = FFI_SYSV
        }

    } else {
        pragma(msg, "Unsupported OS.");
        static assert(1 == 0);
    }

} else version (PPC) {
    version (linux) {
        enum ffi_abi {
            FFI_FIRST_ABI = 0,
            FFI_SYSV,
            FFI_GCC_SYSV,
            FFI_LINUX64,
            FFI_LINUX,
            FFI_DEFAULT_ABI = FFI_LINUX
        }

    } else version (aix) {
        enum ffi_abi {
            FFI_FIRST_ABI = 0,
            FFI_AIX,
            FFI_DARWIN,
            FFI_DEFAULT_ABI = FFI_AIX
        }

    } else version (darwin) {
        enum ffi_abi {
            FFI_FIRST_ABI = 0,
            FFI_AIX,
            FFI_DARWIN,
            FFI_DEFAULT_ABI = FFI_DARWIN
        }

    } else version (freebsd) {
        enum ffi_abi {
            FFI_FIRST_ABI = 0,
            FFI_SYSV,
            FFI_GCC_SYSV,
            FFI_LINUX64,
            FFI_DEFAULT_ABI = FFI_SYSV
        }

    } else {
        pragma(msg, "Unsupported OS.");
        static assert(1 == 0);
    }

} else version (SPARC64) {
    enum ffi_abi {
        FFI_FIRST_ABI = 0,
        FFI_V8,
        FFI_V8PLUS,
        FFI_V9,
        FFI_DEFAULT_ABI = FFI_V9
    }

} else version (SPARC) {
    enum ffi_abi {
        FFI_FIRST_ABI = 0,
        FFI_V8,
        FFI_V8PLUS,
        FFI_V9,
        FFI_DEFAULT_ABI = FFI_V8
    }

}

struct ffi_cif {
  ffi_abi abi;
  uint nargs;
  ffi_type **arg_types;
  ffi_type *rtype;
  uint bytes;
  uint flags;
/*#ifdef FFI_EXTRA_CIF_FIELDS
  FFI_EXTRA_CIF_FIELDS;
#endif*/
}

ffi_status ffi_prep_cif(ffi_cif *cif,
			ffi_abi abi,
			uint nargs,
			ffi_type *rtype,
			ffi_type **atypes);

void ffi_call(ffi_cif *cif,
	      void *fn,
	      void *rvalue,
	      void **avalue);

const int FFI_TYPE_VOID = 0;
const int FFI_TYPE_INT = 1;
const int FFI_TYPE_FLOAT = 2;
const int FFI_TYPE_DOUBLE = 3;
const int FFI_TYPE_LONGDOUBLE = 4;
const int FFI_TYPE_UINT8 = 5;
const int FFI_TYPE_SINT8 = 6;
const int FFI_TYPE_UINT16 = 7;
const int FFI_TYPE_SINT16 = 8;
const int FFI_TYPE_UINT32 = 9;
const int FFI_TYPE_SINT32 = 10;
const int FFI_TYPE_UINT64 = 11;
const int FFI_TYPE_SINT64 = 12;
const int FFI_TYPE_STRUCT = 13;
const int FFI_TYPE_POINTER = 14;
const int FFI_TYPE_LAST = FFI_TYPE_POINTER;

}
