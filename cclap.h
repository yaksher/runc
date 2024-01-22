#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>

#ifndef CCLAP_PREFIXED
#define CCLAP_PREFIXED(TOK) cclap_ ## TOK
#endif

#define ARGS_T CCLAP_PREFIXED(args_t)
#define PARSE_POSITIONAL CCLAP_PREFIXED(_parse_positional_)
#define PARSE_LONG CCLAP_PREFIXED(_parse_long_)
#define PARSE_SHORT CCLAP_PREFIXED(_parse_short_)
#define PARSE_NAMED CCLAP_PREFIXED(_parse_named_)
#define FPRINT_DESCRIPTIONS CCLAP_PREFIXED(fprint_descriptions)
#define PRINT_DESCRIPTIONS CCLAP_PREFIXED(print_descriptions)
#define EPRINT_DESCRIPTIONS CCLAP_PREFIXED(eprint_descriptions)
#define ARGS_DESTROY CCLAP_PREFIXED(args_destroy)
#define PARSE CCLAP_PREFIXED(parse)
#define IS_BOOL(TYPE) (strcmp(#TYPE, "bool") == 0 || strcmp(#TYPE, "_Bool") == 0)


#define POSITIONAL(TYPE, NAME, ...) TYPE NAME;
#define NAMED(TYPE, NAME, ...) TYPE NAME;
#define NAMED_SHORT NAMED
#define NAMED_LONG NAMED
typedef struct ARGS_T {
    char *proccess_name;
    CCLAP_ARGS
    size_t num_extra;
    char **extra;
} ARGS_T;
#undef POSITIONAL
#undef NAMED
#undef NAMED_SHORT
#undef NAMED_LONG

static int PARSE_POSITIONAL(ARGS_T *args) {
    char **extra = args->extra;

#define NAMED(...)
#define NAMED_LONG NAMED
#define NAMED_SHORT NAMED
#define POSITIONAL(TYPE, NAME, ...)\
    if (*extra == NULL) {                               \
        goto EXIT;                                      \
    }                                                   \
    if (strcmp(*extra, "--") == 0) {                    \
        free(*extra);                                   \
        *extra = NULL;                                  \
        extra++;                                        \
        goto EXIT;                                      \
    }                                                   \
    if (strcmp(#TYPE, "char *") == 0) {                 \
        *(char **) &args->NAME = *extra;                \
    }                                                   \
    if (strcmp(#TYPE, "long *") == 0) {                 \
        char *end;                                      \
        long val = strtol(*extra, &end, 10);            \
        if (*end != '\0') {                             \
            return -1;                                  \
        }                                               \
        free(*extra);                                   \
        *extra = NULL;                                  \
        *(long**)&args->NAME = malloc(sizeof(long));    \
        if (!args->NAME) {                              \
            return -1;                                  \
        }                                               \
        *(long *) args->NAME = val;                     \
        return 2;                                       \
    }                                                   \
    extra++;

    CCLAP_ARGS

#undef POSITIONAL
#undef NAMED
#undef NAMED_SHORT
#undef NAMED_LONG

    EXIT:
    return extra - args->extra;
}

static int PARSE_LONG(ARGS_T *args, char *argv[]) {
#define POSITIONAL(...)
#define NAMED_SHORT(...)
#define NAMED_LONG NAMED
#define NAMED(TYPE, NAME, ...)\
    if (strcmp(argv[0], "--" #NAME) == 0) {                 \
        if (strcmp(#TYPE, "char *") == 0) {                 \
            if (argv[1] == NULL) {                          \
                return -1;                                  \
            } else {                                        \
                *(char **) &args->NAME = strdup(argv[1]);   \
                if (!args->NAME) {                          \
                    return -1;                              \
                }                                           \
                return 2;                                   \
            }                                               \
        }                                                   \
        if (strcmp(#TYPE, "long *") == 0) {                 \
            if (argv[1] == NULL) {                          \
                return -1;                                  \
            } else {                                        \
                char *end;                                  \
                long val = strtol(argv[1], &end, 10);       \
                if (*end != '\0') {                         \
                    return -1;                              \
                }                                           \
                *(long**)&args->NAME = malloc(sizeof(long));\
                if (!args->NAME) {                          \
                    return -1;                              \
                }                                           \
                *(long *) args->NAME = val;                 \
                return 2;                                   \
            }                                               \
        }                                                   \
        if (IS_BOOL(TYPE)) {                                \
            *(bool *) &args->NAME = true;                   \
            return 1;                                       \
        }                                                   \
    }

    CCLAP_ARGS

#undef POSITIONAL
#undef NAMED
#undef NAMED_SHORT
#undef NAMED_LONG

    args->extra[args->num_extra++] = strdup(argv[0]);
    if (args->extra[args->num_extra - 1] == NULL) {
        return -1;
    }
    return 1;
}

static int PARSE_SHORT(ARGS_T *args, char *argv[]) {
    char unused[ARG_MAX] = { 0 };
    size_t unused_i = 0;
    int args_i = 1;
    for (char *s = *argv + 1; *s;) {

#define POSITIONAL(...)
#define NAMED_LONG(...)
#define NAMED_SHORT NAMED
#define NAMED(TYPE, NAME, SHORT, ...)\
    if (*s == SHORT) {                                      \
        s++;                                                \
        if (strcmp(#TYPE, "char *") == 0) {                 \
            if (argv[args_i] == NULL) {                     \
                return -1;                                  \
            } else {                                        \
                *(char**)&args->NAME = strdup(argv[args_i]);\
                if (!args->NAME) {                          \
                    return -1;                              \
                }                                           \
                args_i++;                                   \
            }                                               \
        }                                                   \
        if (strcmp(#TYPE, "long *") == 0) {                 \
            if (argv[args_i] == NULL) {                     \
                return -1;                                  \
            } else {                                        \
                char *end;                                  \
                long val = strtol(argv[1], &end, 10);       \
                if (*end != '\0') {                         \
                    return -1;                              \
                }                                           \
                *(long**)&args->NAME = malloc(sizeof(long));\
                if (!args->NAME) {                          \
                    return -1;                              \
                }                                           \
                *(long *) args->NAME = val;                 \
                args_i++;                                   \
            }                                               \
        }                                                   \
        if (IS_BOOL(TYPE)) {                                \
            *(bool *) &args->NAME = true;                   \
        }                                                   \
        continue;                                           \
    }

        CCLAP_ARGS

#undef POSITIONAL
#undef NAMED
#undef NAMED_SHORT
#undef NAMED_LONG

        unused[unused_i++] = *s++;
    }
    if (unused_i > 0) {
        char *unused_heap = calloc(unused_i + 2, sizeof(char));
        if (unused_heap == NULL) {
            return -1;
        }
        *unused_heap = '-';
        strlcat(unused_heap, unused, unused_i + 2);
        args->extra[args->num_extra++] = unused_heap;
    }
    return args_i;
}

static int PARSE_NAMED(ARGS_T *args, char *argv[]) {
    if (argv[0][1] == '-') {
        return PARSE_LONG(args, argv);
    } else {
        return PARSE_SHORT(args, argv);
    }
}

/**
 * @brief Destroys an `ARGS_T` struct returned by `PARSE`. Look at documentation
 * of `CCLAP_PREFIXED(parse)` for more information. `NULL` fields are skipped, so
 * ownership can be transferred out without double free by setting fields to
 * `NULL` before passing them to this function.
 */
static void ARGS_DESTROY(ARGS_T *args) {
    if (args == NULL) {
        return;
    }
    free(args->proccess_name);
    #define FREE(TYPE, NAME, ...)               \
        if (!IS_BOOL(TYPE)) {                   \
            if ((void *) args->NAME != NULL) {  \
                free((void *) args->NAME);      \
            }                                   \
        }
    #define POSITIONAL FREE
    #define NAMED FREE
    #define NAMED_SHORT FREE
    #define NAMED_LONG FREE
    CCLAP_ARGS
    #undef FREE
    #undef POSITIONAL
    #undef NAMED
    #undef NAMED_SHORT
    #undef NAMED_LONG
    if (args->extra != NULL) {
        for (size_t i = 0; i < args->num_extra; i++) {
            free(args->extra[i]);
        }
        free(args->extra);
    }
    free(args);
}

#define CLAP_MAX_SUPPORTED_ARG_COUNT 1024

/**
 * @brief Takes up to 1024 command line arguments and parses them.
 * 
 * The name of this function is generated by `CCLAP_PREFIXED(parse)`, which is
 * defined by default to add the prefix `cclap_`. It is however, possible to
 * include this file multiple times, redefining CCLAP_PREFIXED to a different
 * prefix each time.
 * 
 * It returns a value of type `ARGS_T *`, which is defined to be
 * `CCLAP_PREFIXED(args_t) *`.
 * 
 * The PARSE and ARGS_T macros are undefined within this file; in order to use
 * this function, you must use the expanded value, as governed by `CCLAP_PREFIXED`
 * 
 * The pointer returned by `PARSE` should be passed to `ARGS_DESTROY`
 * (`CCLAP_PREFIXED(args_destroy)`), which will deallocate all memory allocated by
 * it. All pointers are heap allocated using `malloc.` Ownership of the pointers
 * can be transferred out of the `ARGS_T` struct without double free by setting
 * the field in the struct to `NULL`.
 * 
 * `PARSE` will return `NULL` in one of three cases: 
 * - an argument (positional or named) which expected a number got an argument 
 *      that could not fully be parsed as a number
 * - a non-bool named option was present without a corresponding argument to
 *      read as its value.
 * - an internal allocation fails
 * 
 * See `readme.md` and `example.c` for more information.
 * 
 * @return ARGS_T* A nullable pointer to a struct containing the parsed args.
 */
static ARGS_T *PARSE(int argc, char *argv[]) {
    if (argc > CLAP_MAX_SUPPORTED_ARG_COUNT) {
        return NULL;
    }
    ARGS_T *args = calloc(1, sizeof(ARGS_T));
    if (args == NULL) {
        return NULL;
    }
    char *extra[CLAP_MAX_SUPPORTED_ARG_COUNT + 1] = { 0 };
    args->extra = extra;
    args->proccess_name = strdup(*argv++);
    bool rest_extra = false;
    while (*argv != NULL) {
        if (!rest_extra && **argv == '-') {
            if (strcmp(*argv, "--") == 0) {
                // since we don't increment argv, the `--` will be added to
                // the extra array on the next iteration.
                rest_extra = true;
                continue;
            }
            int num_read = PARSE_NAMED(args, argv);
            if (num_read == -1) {
                goto FAIL;
            }
            argv += num_read;
        } else {
            args->extra[args->num_extra++] = strdup(*argv++);
        }
    }
    int num_pos_read = PARSE_POSITIONAL(args);
    if (num_pos_read == -1) {
        goto FAIL;
    }

    size_t num_extra = args->num_extra - num_pos_read;
    char **extra_heap = calloc(num_extra + 1, sizeof(char *));
    for (size_t i = 0; i < num_extra; i++) {
        extra_heap[i] = args->extra[i + num_pos_read];
    }
    args->extra = extra_heap;
    args->num_extra = num_extra;

    return args;
    FAIL:
    for (size_t i = 0; i < args->num_extra; i++) {
        free(extra[i]);
    }
    args->extra = NULL;
    ARGS_DESTROY(args);
    return NULL;
}

#define _GET_SECOND(A, B, ...) B
#define GET_DESCRIPTION(A, B, C, D...) _GET_SECOND(A, ## D, "")

/**
 * @brief Prints the descriptions of all options, along with their names and
 * type information (positional arguments are specified as [positional] for
 * their name) to the provided `stream`.
 */
__attribute__((unused))
static void FPRINT_DESCRIPTIONS(FILE *stream) {
    fprintf(stream, "Available options are:\n");
#define TYPE_STR(TYPE) (strcmp(#TYPE, "char *") == 0 ? "string" :    \
                        strcmp(#TYPE, "long *") == 0 ? "long" :      \
                        IS_BOOL(TYPE) ? "flag" : "unknown")
#define PRINT_DESC(DESC) \
    if (DESC[0] != '\0') {                                              \
        fprintf(stream, "\t| %s\n", DESC);                              \
    } else {                                                            \
        fprintf(stream, "\n");                                          \
    }

#define NAMED_(TYPE, NAME, SHORT, DESC) do {\
        const char *type_str = TYPE_STR(TYPE);                              \
        fprintf(stream, "\t--"#NAME", -%c: %s", SHORT, type_str);           \
        PRINT_DESC(DESC)                                                    \
    } while(0);
#define NAMED(T, N, S, MAYBE_DESC...) NAMED_(T, N, S, GET_DESCRIPTION(T, N, S, ##MAYBE_DESC))

#define NAMED_LONG_(TYPE, NAME, DESC) do {\
        const char *type_str = TYPE_STR(TYPE);                              \
        fprintf(stream, "\t--"#NAME": %s", type_str);                       \
        PRINT_DESC(DESC)                                                    \
    } while(0);
#define NAMED_LONG(T, N, MAYBE_DESC...) NAMED_LONG_(T, N, GET_DESCRIPTION(T, N, '_', ##MAYBE_DESC))

#define NAMED_SHORT_(TYPE, SHORT, DESC) do {\
        const char *type_str = TYPE_STR(TYPE);                              \
        fprintf(stream, "\t-%c: %s", SHORT, type_str);                      \
        PRINT_DESC(DESC)                                                    \
    } while(0);
#define NAMED_SHORT(T, N, S, MAYBE_DESC...) NAMED_SHORT_(T, S, GET_DESCRIPTION(T, N, S, ##MAYBE_DESC))

#define POSITIONAL_(TYPE, DESC) do {\
        const char *type_str = TYPE_STR(TYPE);                              \
        fprintf(stream, "\t[positional]: %s", type_str);                    \
        PRINT_DESC(DESC)                                                    \
    } while(0);
#define POSITIONAL(T, N, MAYBE_DESC...) POSITIONAL_(T, GET_DESCRIPTION(T, N, '_', ##MAYBE_DESC))

    CCLAP_ARGS

#undef POSITIONAL
#undef POSITIONAL_
#undef NAMED
#undef NAMED_
#undef NAMED_LONG
#undef NAMED_LONG_
#undef NAMED_SHORT
#undef NAMED_SHORT_
#undef PRINT_DESC
#undef TYPE_STR
}

/**
 * @brief Prints the descriptions of all options, along with their names and
 * type information (positional arguments are specified as [positional] for
 * their name) to stdout.
 */
__attribute__((unused))
static void PRINT_DESCRIPTIONS() {
    FPRINT_DESCRIPTIONS(stdout);
}

/**
 * @brief Prints the descriptions of all options, along with their names and
 * type information (positional arguments are specified as [positional] for
 * their name) to stderr.
 */
__attribute__((unused))
static void EPRINT_DESCRIPTIONS() {
    FPRINT_DESCRIPTIONS(stderr);
}

#undef _GET_SECOND
#undef GET_DESCRIPTION

#undef ARGS_T
#undef PARSE_POSITIONAL
#undef PARSE_LONG
#undef PARSE_SHORT
#undef PARSE_NAMED
#undef ARGS_DESTROY
#undef PARSE
#undef IS_BOOL