#ifndef DIFFHEX_H
#define DIFFHEX_H
/* Diffhex - Hex diffs and dumps of arrays in a header-only-library.
 * This file is in the Public Domain.
 * https://github.com/agvxov/diffhex-h
 */

#include <ctype.h>
#include <assert.h>

/* Printf like printing call back for all output operations.
 */
typedef int (*printf_fn)(const char *, ...);

/* Declare printf such that we can use it as the default print function.
 * We don't need it otherwise.
 */
extern int printf(const char *, ...);

typedef struct {
    size_t quads_per_line;
    bool is_absolute_address;
    bool do_collapse_identical;
    printf_fn print;
    const char * color_reset;
    const char * color_address;
    const char * color_match;
    const char * color_diff;
    const char * color_extra;
} diffhex_options_t;

diffhex_options_t diffhex_defaults = {
    .quads_per_line        = 1,
    .is_absolute_address   = false,
    .do_collapse_identical = false,
    .print                 = printf,
    .color_reset           = "\033[0m",
    .color_address         = "\033[36m",
    .color_match           = "",
    .color_diff            = "\033[33m",
    .color_extra           = "\033[31m",
};

extern void diffhex(const void * buffer1, size_t size1, const void * buffer2, size_t size2, diffhex_options_t options);

static inline
void dumphex(const void * buffer, size_t size, diffhex_options_t options) {
    diffhex(buffer, size, buffer, size, options);
}

#ifdef DIFFHEX_IMPLEMENTATION

static inline
bool diffhex_line_equal(
    const unsigned char * b1,
    size_t size1,
    const unsigned char * b2,
    size_t size2,
    size_t offset,
    size_t len
) {
    if (offset + len > size1
    ||  offset + len > size2) {
        return false;
    }

    for (size_t i = 0; i < len; i++) {
        if (b1[offset + i] != b2[offset + i]) {
            return false;
        }
    }

    return true;
}

static
void diffhex_print_address(
    const void * buffer1,
    size_t offset,
    diffhex_options_t options
) {
    size_t display_offset;
    if (options.is_absolute_address) {
        display_offset = (size_t)buffer1 + offset;
    } else {
        display_offset = offset;
    }
    options.print("%s0x%08zx%s ", options.color_address, display_offset, options.color_reset);
}

static
void diffhex_print_ascii(
    const unsigned char * const b1,
    size_t size1,
    const unsigned char * const b2,
    size_t size2,
    size_t bytes_per_line,
    size_t max_size,
    size_t offset,
    diffhex_options_t options
) {
    for (size_t i = 0; i < bytes_per_line; i++) {
        size_t index = offset + i;

        if (index >= max_size) {
            break;
        }

        if (index >= size1) {
            options.print("%s?%s", options.color_extra, options.color_reset);
            continue;
        }

        unsigned char c = b1[index];
        char out = isprint(c) ? c : '.';

        const char * color = options.color_match;

        if (index >= size2) {
            color = options.color_extra;
        } else
        if (b1[index] != b2[index]) {
            color = options.color_diff;
        }

        if (*color) {
            options.print("%s%c%s", color, out, options.color_reset);
        } else {
            options.print("%c", out);
        }
    }
}

void diffhex(
    const void * buffer1,
    size_t size1,
    const void * buffer2,
    size_t size2,
    diffhex_options_t options
) {
    assert(options.quads_per_line != 0);

    const unsigned char * const b1 = buffer1;
    const unsigned char * const b2 = buffer2;

    const size_t bytes_per_line = 8 * options.quads_per_line;

    size_t max_size = size1 > size2 ? size1 : size2;

    bool is_prev_equal = false;

    for (size_t offset = 0; offset < max_size; offset += bytes_per_line) {
        if (options.do_collapse_identical) {
            bool is_equal = diffhex_line_equal(b1, size1, b2, size2, offset, bytes_per_line);
            if (is_equal) {
                if (is_prev_equal) {
                    continue;
                } else {
                    is_prev_equal = true;

                    diffhex_print_address(buffer1, offset, options);

                    options.print("%s [ discarded identical ]  ", options.color_match);
                    for (size_t i = 0; i < options.quads_per_line-1; i++) {
                        options.print("-- -- -- -- -- -- -- --  ");
                    }
                    options.print("%s", options.color_reset);

                    diffhex_print_ascii(b1, size1, b2, size2, bytes_per_line, max_size, offset, options);

                    options.print("\n");

                    continue;
                }
            } else {
                is_prev_equal = false;
            }
        }

        // Address
        diffhex_print_address(buffer1, offset, options);

        // Hex
        for (size_t i = 0; i < bytes_per_line; i++) {
            if (i % 8 == 0) {
                options.print(" ");
            }

            size_t index = offset + i;

            if (index >= max_size) {
                options.print("   ");
                continue;
            }

            if (index >= size1) {
                /* buffer2 is longer */
                options.print("%s??%s ", options.color_extra, options.color_reset);
                continue;
            }

            const char * color = options.color_match;

            if (index >= size2) {
                color = options.color_extra;
            } else
            if (b1[index] != b2[index]) {
                color = options.color_diff;
            }

            if (*color) {
                options.print("%s%02X%s ", color, b1[index], options.color_reset);
            } else {
                options.print("%02X ", b1[index]);
            }
        }

        options.print(" ");

        // ASCII
        diffhex_print_ascii(b1, size1, b2, size2, bytes_per_line, max_size, offset, options);

        options.print("\n");
    }
}
#endif

#endif
