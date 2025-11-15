# Slurp
> Perl's File::Slurp in C

### Functionality
This header provides functions that allow you to read or write entire files
with one simple call.
It was modelled after the
[Perl module of the same name](https://metacpan.org/pod/File::Slurp).

The following are provided:
```C
static char * read_file(const char * const path);
static inline int write_file(const char * const path, const char * const s);
static inline int overwrite_file(const char * const path, const char * const s);
static inline int append_file(const char * const path, const char * const s);
static inline int prepend_file(const char * const path, const char * const s);

#define slurp read_file
```

Short-hand aliases have been dropped, namely: `ef, efl, rf, wf`.

There is no `edit_file/edit_file_lines` as that would be extremely convoluted.

There is no `read_dir` because it was judged to not fit the rest of the library.

### Notes
`write_file` only succeds if the file does not exist.

`prepend_file` merely wraps the other functions, meaning it could be optimized.

### Portability
Currently only Linux is supported.

### Todo
* support other platforms
* file descriptor variants with generic wrappers
* line oriented variants
* support for `FILE*` like construct -such as `FCGX_Stream*`- with metaprogramming
