#include <kstd/cstdio.hpp>
#include <ioports.hpp>

static inline int num_digits(u64 x, u64 b = 10) {
    int i = 0;
    while (x /= b) i++;
    return i;
}

namespace kstd {

int putchar(char c) {
    io::outb(0x3F8, c);
    return c;
}

int vprintf(const char *format, va_list list) {
    int written = 0;
    for (int i = 0; format[i]; i++) {
        bool alt_form = false;
        bool long_int = false;
        u8 zero_pad = 0;

        if (format[i] == '%') {
            i++;

            while (1) {
                switch (format[i]) {
                case '#':
                    alt_form = true;
                    i++;
                    break;
                case '0':
                    i++;
                    zero_pad = format[i] - '0';
                    i++;
                    if (format[i] >= '0' && format[i] <= '9') {
                        zero_pad = ((format[i - 1] - '0') * 10) + (format[i] - '0');
                        i++;
                    }
                    break;
                case 'l':
                    long_int = true;
                    i++;
                    break;
                case 'x':
                case 'X': {
                    if (alt_form) {
                        putchar('0');
                        putchar('x');
                        written += 2;
                    }
                    const char *low = "0123456789abcdef";
                    const char *high = "0123456789ABCDEF";
                    const char *used = format[i] == 'X' ? high : low;
                    u64 value = 0;
                    if (long_int) value = va_arg(list, u64);
                    else value = va_arg(list, u32);
                    int digits = num_digits(value, 16);
                    if (zero_pad && zero_pad > digits) {
                        for (int j = 0; j < zero_pad - digits - 1; j++)
                            putchar('0');
                        written += zero_pad - digits - 1;
                    }
                    for (int di = digits; di >= 0; di--) {
                        u64 v = value;
                        for (int dii = 0; dii < di; dii++) v /= 16;
                        u64 d = v % 16;
                        putchar(used[d]);
                        written++;
                    }
                    goto end;
                }
                case 'd': {
                    u64 value = 0;
                    if (long_int) value = va_arg(list, u64);
                    else value = va_arg(list, u32);
                    u64 v = value;
                    int digits = num_digits(value);
                    if (zero_pad && zero_pad > digits) {
                        for (int j = 0; j < zero_pad - digits - 1; j++)
                            putchar('0');
                        written += zero_pad - digits - 1;
                    }
                    for (int di = digits; di >= 0; di--) {
                        u64 v = value;
                        for (int dii = 0; dii < di; dii++) v /= 10;
                        u64 d = v % 10;
                        putchar('0' + d);
                        written++;
                    }
                    goto end;
                }
                case 's': {
                    const char *str = va_arg(list, const char*);
                    for (int i = 0; str[i]; i++) {
                        putchar(str[i]);
                        written++;
                    }
                    goto end;
                }
                }
            }
            end:
            continue;
        }

        written++;
        putchar(format[i]);
    }
    return written;
}

__attribute__((format(printf, 1, 2))) int printf(const char *format, ...) {
    va_list list;
    va_start(list, format);
    int i = vprintf(format, list);
    va_end(list);
    return i;
}

}
