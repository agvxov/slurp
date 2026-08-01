// @BAKE clang -I../ -o $*.out $@ -lcriterion -Wall -Wpedantic -Wextra -O0 -ggdb -std=c23 -fsanitize=address,undefined
#define SLURP_IMPLEMENTATION
#include "slurp.h"
#include <criterion/criterion.h>
#include <errno.h>
#define DIFFHEX_IMPLEMENTATION
#include "diffhex.h"

/* NOTE:
 * Do not, I repeat, DO NOT even sneed about reusing the same file name between tests.
 * That creates a system level race condition as each test case is its own process.
 */

/*  _  _     _
 * | || |___| |_ __  ___ _ _ ___
 * | __ / -_) | '_ \/ -_) '_(_-<
 * |_||_\___|_| .__/\___|_| /__/
 *            |_|
 */
typedef struct {
    char * path;
    char * expected_contents;
    size_t expected_size; // expected_contents may contain '\0'
} my_file_t;

#define construct_my_file(path, data) \
    (my_file_t) {                     \
        path,                         \
        data,                         \
        sizeof(data)-1                \
    }

static
void write_fixture(const char * path, const char * contents, size_t size) {
    unlink(path);
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    cr_assert(fd != -1, "failed to create fixture %s", path);
    if (size > 0) {
        ssize_t w = write(fd, contents, size);
        cr_assert(w == (ssize_t)size, "short write while creating fixture %s", path);
    }
    close(fd);
}

static
void cleanup(const char * path) {
    unlink(path);
}

static
void cr_assert_feq(const char * s, my_file_t f) {
    cr_assert(!strcmp(s, f.expected_contents));
}

static
void cr_assert_memcmp_dumping(void * a, size_t size_a, void * b, size_t size_b) {
    int memory_comperasion = 1;

    if (size_a != size_b) {
        goto diff;
    }

    memory_comperasion = memcmp(a, b, size_a);

    if (memory_comperasion) {
      diff:
        auto opts = diffhex_defaults;
        opts.quads_per_line = 2;
        opts.color_match = "\033[32m";
        diffhex(a, size_a, b, size_b, opts);
        cr_assert(!memory_comperasion);
    }

}

/*  ___             _ _
 * | _ \___ __ _ __| (_)_ _  __ _
 * |   / -_) _` / _` | | ' \/ _` |
 * |_|_\___\__,_\__,_|_|_||_\__, |
 *                          |___/
 */
Test(read_file, basic) {
    write_fixture("test.txt", "This is my test input file.\n", 29);
    char * s = read_file("test.txt");
    cr_assert(!strcmp(s, "This is my test input file.\n"));
    free(s);
}

Test(read_file, basic2) {
    my_file_t f = construct_my_file(
        "t_read_basic.txt",
        "This is my test input file.\n"
    );
    write_fixture(f.path, f.expected_contents, f.expected_size);

    char * s = read_file(f.path);
    cr_assert(s != NULL);
    cr_assert_feq(s, f);

    free(s);
    cleanup(f.path);
}

Test(read_file, empty_file) {
    my_file_t f = construct_my_file(
        "t_read_empty.txt",
        ""
    );
    write_fixture(f.path, f.expected_contents, f.expected_size);

    size_t size;
    char * s = read_file_get_size(f.path, &size);
    cr_assert(s != NULL);
    cr_assert(size == 0);
    cr_assert(s[0] == '\0');

    free(s);
    cleanup(f.path);
}

Test(read_file, missing_file_returns_null) {
    cleanup("t_read_missing.txt");
    char * s = read_file("t_read_missing.txt");
    cr_assert(s == NULL);
}

Test(read_file, embedded_null_bytes) {
    my_file_t f = construct_my_file(
        "t_read_nulbytes.bin",
        "ab\0cd"
    );
    write_fixture(f.path, f.expected_contents, f.expected_size);

    size_t size;
    char * s = read_file_get_size(f.path, &size);
    cr_assert(s != NULL);
    cr_assert_memcmp_dumping(s, size, f.expected_contents, f.expected_size);

    free(s);
    cleanup(f.path);
}

Test(read_file, large_file) {
    // Big enough it cannot be slurped in one read() call
    size_t size = 1024 * 1024 + 37;
    char * buf = malloc(size);
    cr_assert(buf != NULL);
    for (size_t i = 0; i < size; i++) {
        buf[i] = (char)('A' + (i % 26));
    }

    my_file_t f = { "t_read_large.bin", buf, size };
    write_fixture(f.path, f.expected_contents, f.expected_size);

    size_t got;
    char * s = read_file_get_size(f.path, &got);
    cr_assert(s != NULL);
    cr_assert(got == size);
    cr_assert_memcmp_dumping(s, got, buf, size);

    free(s);
    free(buf);
    cleanup(f.path);
}

Test(read_file, dev_null) {
    size_t size;
    char * s = read_file_get_size("/dev/null", &size);
    cr_assert(s != NULL);
    cr_assert(size == 0);
    free(s);
}

/* __      __   _ _   _
 * \ \    / / _(_) |_(_)_ _  __ _
 *  \ \/\/ / '_| |  _| | ' \/ _` |
 *   \_/\_/|_| |_|\__|_|_||_\__, |
 *                          |___/
 */
Test(write_file, basic) {
    unlink("test.out.txt");
    cr_assert(!write_file("test.out.txt", "test"));
    cleanup("test.out.txt");
}

Test(write_file, creates_new_file_with_contents) {
    my_file_t f = construct_my_file(
        "t_write_new.txt",
        "fresh content"
    );
    cleanup(f.path);

    cr_assert(!write_file(f.path, f.expected_contents));

    char * s = read_file(f.path);
    cr_assert(s != NULL);
    cr_assert_feq(s, f);

    free(s);
    cleanup(f.path);
}

Test(write_file, fails_and_does_not_clobber_if_file_exists) {
    my_file_t f = construct_my_file(
        "t_write_exists.txt",
        "original"
    );
    write_fixture(f.path, f.expected_contents, f.expected_size);

    int rc = write_file(f.path, "should not overwrite");
    cr_assert(rc != 0);

    char * s = read_file(f.path);
    cr_assert(s != NULL);
    cr_assert_feq(s, f);

    free(s);
    cleanup(f.path);
}

Test(write_file, empty_string_content) {
    my_file_t f = construct_my_file(
        "t_write_empty.txt",
        ""
    );
    cleanup(f.path);

    int rc = write_file(f.path, f.expected_contents);
    cr_assert(!rc, "write_file with empty content failed, errno=%d (%s)", errno, strerror(errno));

    size_t size;
    char * s = read_file_get_size(f.path, &size);
    cr_assert(s != NULL);
    cr_assert(size == 0);

    free(s);
    cleanup(f.path);
}

/*   ___            __      __   _ _   _
 *  / _ \__ _____ _ \ \    / / _(_) |_(_)_ _  __ _
 * | (_) \ V / -_) '_\ \/\/ / '_| |  _| | ' \/ _` |
 *  \___/ \_/\___|_|  \_/\_/|_| |_|\__|_|_||_\__, |
 *                                           |___/
 */
Test(overwrite_file, basic) {
    cr_assert(!overwrite_file("test.out.txt", "test"));
    cleanup("test.out.txt");
}

Test(overwrite_file, creates_file_if_missing) {
    my_file_t f = construct_my_file(
        "t_overwrite_new.txt",
        "created via overwrite"
    );
    cleanup(f.path);

    cr_assert(!overwrite_file(f.path, f.expected_contents));

    char * s = read_file(f.path);
    cr_assert(s != NULL);
    cr_assert_feq(s, f);

    free(s);
    cleanup(f.path);
}

Test(overwrite_file, truncates_when_new_content_is_shorter) {
    my_file_t f = construct_my_file(
        "t_overwrite_shrink.txt",
        "short"
    );
    write_fixture(f.path, "this was a much longer original string", 39);

    cr_assert(!overwrite_file(f.path, f.expected_contents));

    size_t size;
    char * s = read_file_get_size(f.path, &size);
    cr_assert(s != NULL);
    cr_assert(size == f.expected_size, "leftover bytes from old content were not truncated");
    cr_assert_feq(s, f);

    free(s);
    cleanup(f.path);
}

Test(overwrite_file, extends_if_new_content_is_longer) {
    my_file_t f = construct_my_file(
        "t_overwrite_grow.txt",
        "a considerably longer replacement string"
    );
    write_fixture(f.path, "short", 5);

    cr_assert(!overwrite_file(f.path, f.expected_contents));

    size_t size;
    char * s = read_file_get_size(f.path, &size);
    cr_assert(s != NULL);
    cr_assert(size == f.expected_size);
    cr_assert_feq(s, f);

    free(s);
    cleanup(f.path);
}

Test(overwrite_file, write_stdout) {
    cr_assert(!overwrite_file("/dev/stdout", "test"));
}

Test(overwrite_file, empty_string_content) {
    my_file_t f = construct_my_file(
        "t_overwrite_empty.txt",
        ""
    );
    write_fixture(f.path, "old content that should be cleared", 35);

    int rc = overwrite_file(f.path, f.expected_contents);
    cr_assert(!rc, "overwrite_file with empty content failed, errno=%d (%s)", errno, strerror(errno));

    size_t size;
    char * s = read_file_get_size(f.path, &size);
    cr_assert(s != NULL);
    cr_assert(size == 0);

    free(s);
    cleanup(f.path);
}

/*    _                          _ _
 *   /_\  _ __ _ __  ___ _ _  __| (_)_ _  __ _
 *  / _ \| '_ \ '_ \/ -_) ' \/ _` | | ' \/ _` |
 * /_/ \_\ .__/ .__/\___|_||_\__,_|_|_||_\__, |
 *       |_|  |_|                        |___/
 */
Test(append_file, creates_file_if_missing) {
    my_file_t f = construct_my_file(
        "t_append_new.txt",
        "first content"
    );
    cleanup(f.path);

    cr_assert(!append_file(f.path, f.expected_contents));

    char * s = read_file(f.path);
    cr_assert(s != NULL);
    cr_assert_feq(s, f);

    free(s);
    cleanup(f.path);
}

Test(append_file, appends_after_existing_content) {
    const char * path = "t_append_existing.txt";
    write_fixture(path, "hello ", 6);

    cr_assert(!append_file(path, "world"));

    char * s = read_file(path);
    cr_assert(s != NULL);
    cr_assert(!strcmp(s, "hello world"));

    free(s);
    cleanup(path);
}

Test(append_file, fallocate_zero_pads_when_existing_content_is_shorter) {
    const char * path = "t_append_padding_bug.txt";
    write_fixture(path, "hi", 2);

    cr_assert(!append_file(path, "world"));

    size_t size;
    char * s = read_file_get_size(path, &size);
    cr_assert(s != NULL);

    char expected[] = "hiworld";
    cr_assert_memcmp_dumping(s, size, expected, strlen(expected));

    free(s);
    cleanup(path);
}

Test(append_file, repeated_appends_accumulate_in_order) {
    const char * path = "t_append_multi.txt";
    cleanup(path);

    cr_assert(!append_file(path, "a"));
    cr_assert(!append_file(path, "b"));
    cr_assert(!append_file(path, "c"));

    char * s = read_file(path);
    cr_assert(s != NULL);
    cr_assert(!strcmp(s, "abc"));

    free(s);
    cleanup(path);
}

Test(append_file, empty_string_content) {
    const char * path = "t_append_empty.txt";
    write_fixture(path, "unchanged", 9);

    int rc = append_file(path, "");
    cr_assert(!rc, "append_file with empty content failed, errno=%d (%s)", errno, strerror(errno));

    char * s = read_file(path);
    cr_assert(s != NULL);
    cr_assert(!strcmp(s, "unchanged"));

    free(s);
    cleanup(path);
}

/*  ___                           _ _
 * | _ \_ _ ___ _ __  ___ _ _  __| (_)_ _  __ _
 * |  _/ '_/ -_) '_ \/ -_) ' \/ _` | | ' \/ _` |
 * |_| |_| \___| .__/\___|_||_\__,_|_|_||_\__, |
 *             |_|                        |___/
 */
Test(prepend_file, prepends_before_existing_content) {
    const char * path = "t_prepend_existing.txt";
    write_fixture(path, "world", 5);

    cr_assert(!prepend_file(path, "hello "));

    char * s = read_file(path);
    cr_assert(s != NULL);
    cr_assert(!strcmp(s, "hello world"));

    free(s);
    cleanup(path);
}

Test(prepend_file, missing_file_is_created) {
    const char * path = "t_prepend_missing.txt";
    cleanup(path);

    int rc = prepend_file(path, "hello");
    cr_assert(!rc);

    char * s = read_file(path);
    cr_assert(s != NULL);
    cr_assert(!strcmp(s, "hello"));

    free(s);
    cleanup(path);
}

/*  ___                  _ _____    _
 * | _ \___ _  _ _ _  __| |_   _| _(_)_ __
 * |   / _ \ || | ' \/ _` | | || '_| | '_ \
 * |_|_\___/\_,_|_||_\__,_| |_||_| |_| .__/
 *                                   |_|
 */
Test(round_trip, write_file_then_read_file) {
    static char rt_content_short[]      = "hi";
    static char rt_content_sentence[]   = "The quick brown fox jumps over the lazy dog.";
    static char rt_content_multiline[]  = "line one\nline two\nline three\n";
    static char rt_content_whitespace[] = "   leading and trailing spaces   ";

    static my_file_t round_trip_cases[] = {
        construct_my_file("t_rt_w_short.txt",      rt_content_short),
        construct_my_file("t_rt_w_sentence.txt",   rt_content_sentence),
        construct_my_file("t_rt_w_multiline.txt",  rt_content_multiline),
        construct_my_file("t_rt_w_whitespace.txt", rt_content_whitespace),
    };

    size_t n = sizeof(round_trip_cases) / sizeof(round_trip_cases[0]);
    for (size_t i = 0; i < n; i++) {
        my_file_t * f = &round_trip_cases[i];
        cleanup(f->path);

        cr_assert(
            !write_file(f->path, f->expected_contents),
            "write_file failed for case %zu (%s)", i, f->path
        );

        size_t size;
        char * s = read_file_get_size(f->path, &size);
        cr_assert(s != NULL, "read back failed for case %zu (%s)", i, f->path);
        cr_assert_memcmp_dumping(s, size, f->expected_contents, f->expected_size);

        free(s);
        cleanup(f->path);
    }
}

Test(round_trip, overwrite_file_then_read_file) {
    static char rt_content_short[]      = "hi";
    static char rt_content_sentence[]   = "The quick brown fox jumps over the lazy dog.";
    static char rt_content_multiline[]  = "line one\nline two\nline three\n";
    static char rt_content_whitespace[] = "   leading and trailing spaces   ";

    static my_file_t round_trip_cases[] = {
        construct_my_file("t_rt_ow_short.txt",      rt_content_short),
        construct_my_file("t_rt_ow_sentence.txt",   rt_content_sentence),
        construct_my_file("t_rt_ow_multiline.txt",  rt_content_multiline),
        construct_my_file("t_rt_ow_whitespace.txt", rt_content_whitespace),
    };

    size_t n = sizeof(round_trip_cases) / sizeof(round_trip_cases[0]);
    for (size_t i = 0; i < n; i++) {
        my_file_t * f = &round_trip_cases[i];
        write_fixture(f->path, "stale placeholder content to be replaced", 41);

        auto e = overwrite_file(f->path, f->expected_contents);

        cr_assert(
            !e,
            "overwrite_file failed for case %zu (%s, %d)",
            i,
            f->path,
            e
        );

        size_t size;
        char * s = read_file_get_size(f->path, &size);
        cr_assert(s != NULL);
        cr_assert_memcmp_dumping(s, size, f->expected_contents, f->expected_size);

        free(s);
        cleanup(f->path);
    }
}

Test(round_trip, embedded_null_bytes) {
    static char rt_binary_content[] = "AB\0CD\0EF";

    my_file_t f = construct_my_file(
        "t_rt_binary.bin",
        rt_binary_content
    );
    cleanup(f.path);

    cr_assert(!write_file_blob(f.path, f.expected_contents, sizeof(rt_binary_content)-1));

    size_t size;
    char * s = read_file_get_size(f.path, &size);
    cr_assert(s != NULL);
    cr_assert_memcmp_dumping(f.expected_contents, sizeof(f.expected_contents), s, size);

    free(s);
    cleanup(f.path);
}
