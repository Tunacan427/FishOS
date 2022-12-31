#include <klib/types.hpp>
#include <panic.hpp>

struct Error {
    const char *file;
    u32 line, column;
};

struct TypeDescriptor {
    u16 kind, info;
    char name[];
};

struct Overflow : Error {
    TypeDescriptor *type;
};

struct ShiftOutOfBounds : Error {
    TypeDescriptor *left_type;
    TypeDescriptor *right_type;
};

struct InvalidValue : Error {
    TypeDescriptor *type;
};

struct ArrayOutOfBounds : Error {
    TypeDescriptor *array_type;
    TypeDescriptor *index_type;
};

struct TypeMismatchV1 : Error {
    TypeDescriptor *type;
    u8 log_alignment, type_check_kind;
};

struct NegativeVLA : Error {
    TypeDescriptor *type;
};

struct NonnullReturn : Error {};
struct NonnullArgument : Error {};
struct Unreachable : Error {};

struct InvalidBuiltin : Error {
    u8 kind;
};

static void handle_error(const char *message, Error err) {
    panic("UBSan Error: %s at %s:%d:%d", message, err.file, err.line, err.column);
}

extern "C" {
    void __ubsan_handle_add_overflow(Overflow *err) {
        handle_error("addition overflow", *err);
    }

    void __ubsan_handle_sub_overflow(Overflow *err) {
        handle_error("subtraction overflow", *err);
    }

    void __ubsan_handle_mul_overflow(Overflow *err) {
        handle_error("multiplication overflow", *err);
    }

    void __ubsan_handle_divrem_overflow(Overflow *err) {
        handle_error("division overflow", *err);
    }

    void __ubsan_handle_negate_overflow(Overflow *err) {
        handle_error("negation overflow", *err);
    }

    void __ubsan_handle_pointer_overflow(Overflow *err) {
        handle_error("pointer overflow", *err);
    }

    void __ubsan_handle_shift_out_of_bounds(ShiftOutOfBounds *err) {
        handle_error("shift out of bounds", *err);
    }

    void __ubsan_handle_load_invalid_value(InvalidValue *err) {
        handle_error("invalid load value", *err);
    }

    void __ubsan_handle_out_of_bounds(ArrayOutOfBounds *err) {
        handle_error("array out of bounds", *err);
    }

    void __ubsan_handle_type_mismatch_v1(TypeMismatchV1 *err, uptr ptr) {
        if (!ptr)
            handle_error("use of null pointer", *err);
        else if (ptr & ((1 << err->log_alignment) - 1))
            handle_error("use of misaligned pointer", *err);
        else
            handle_error("no space for object", *err);
    }

    void __ubsan_handle_vla_bound_not_positive(NegativeVLA *err) {
        handle_error("variable-length argument is negative", *err);
    }

    void __ubsan_handle_nonnull_return(NonnullReturn *err) {
        handle_error("non-null return is null", *err);
    }

    void __ubsan_handle_nonnull_arg(NonnullArgument *err) {
        handle_error("non-null argument is null", *err);
    }

    void __ubsan_handle_builtin_unreachable(Unreachable *err) {
        handle_error("unreachable code reached", *err);
    }

    void __ubsan_handle_invalid_builtin(InvalidBuiltin *err) {
        handle_error("invalid builtin", *err);
    }
}
