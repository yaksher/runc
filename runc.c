#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <assert.h>

#define CCLAP_ARGS \
    NAMED(bool, debug, 'd', "Compiles and runs a debug executable with address sanitizer.") \
    NAMED_LONG(bool, clean, "Clean the build directory.") \
    NAMED(bool, force, 'f', "Force recompilation of target. Useful if headers have changed.") \
    NAMED(char *, cflags, 'c', "Additional flags to pass to compiler. Disables caching.") \
    NAMED(bool, help, 'h', "Prints this description.") \
    POSITIONAL(char *, target, "Target file to compile and run. Should be in the current directory.")
#include "cclap.h"

#if defined(__APPLE__) || defined(__linux__)
    #define COMPILER "cc"
    #define BASE_CFLAGS "-Wall -Wextra"
    #define DEFAULT_DEBUG_CFLAGS    BASE_CFLAGS " -g -fsanitize=address,undefined"
    #define DEFAULT_RELEASE_CFLAGS  BASE_CFLAGS " -O3"
    // const char CONFIG_PATH[] = "~/.config/runc/";
    #define CACHE_DIR ".cache/"
    #define BUILD_DIR CACHE_DIR "runc/"
#else
    #error "Unsupported platform."
#endif
#define BUILD_DIR_LEN (sizeof(BUILD_DIR) - 1)

int main(int argc, char *argv[]) {
    cclap_args_t *args = cclap_parse(argc, argv);
    if (!args) {
        fprintf(stderr, "Usage: %s [options] <target> [-- [target_args]]\n", argv[0]);
        fprintf(stderr, "Try `%s --help` for more information.\n", argv[0]);
        return 1;
    }
    if (args->help) {
        printf("This is simple tool to (compile if necessary and) run C files.\n");
        printf("House keeping is done on every run, deleting build files older than a day.\n");
        printf("By default, executables are compiled for performance.\n");
        printf("You can specify additional compiler flags with `%s --cflags <flags>`, but this will disable caching.\n", args->proccess_name);
        printf("Basic usage: %s [options] <target> [-- [target_args]]\n", args->proccess_name);
        printf("You can clean the build directory with `%s --clean`. This will cause other args to be ignored.\n", args->proccess_name);
        // printf("Config file is located at `%s`.\n", CONFIG_PATH);
        printf("Build directory is located at `%s`.\n", BUILD_DIR);
        cclap_print_descriptions();
        return 0;
    }
    const char *home = getenv("HOME");
    assert(home);
    size_t home_len = strlen(home);
    size_t build_dir_len = home_len + BUILD_DIR_LEN + 1;
    char *build_dir = malloc(build_dir_len + 1);
    assert(build_dir);
    snprintf(build_dir, build_dir_len + 1, "%s/%s", home, BUILD_DIR);
    if (args->clean) {
        printf("Cleaning build directory...\n");
        size_t cmd_size = build_dir_len + strlen("rm -rf ") + 1;
        char *cmd = malloc(cmd_size);
        assert(cmd);
        snprintf(cmd, cmd_size, "rm -rf %s", build_dir);
        system(cmd);
        free(cmd);
        return 0;
    }
    if (!args->target) {
        fprintf(stderr, "No target specified.\n");
        fprintf(stderr, "Try `%s --help` for more information.\n", args->proccess_name);
        return 1;
    }
    char *cflags = 
          args->cflags ? args->cflags 
        : args->debug ? DEFAULT_DEBUG_CFLAGS 
        : DEFAULT_RELEASE_CFLAGS;
    bool cache = !args->cflags; 
    const char *prefix = 
          args->cflags ? "custom_"
        : args->debug ? "debug_"
        : "release_";
    size_t cflags_len = strlen(cflags);
    size_t prefix_len = prefix ? strlen(prefix) : 0;
    char *cache_dir = strdup(build_dir);
    char *cache_dir_end = strrchr(cache_dir, '/');
    cache_dir_end[0] = '\0';
    cache_dir_end = strrchr(cache_dir, '/');
    cache_dir_end[1] = '\0';
    struct stat dir;
    if (stat(cache_dir, &dir) == -1) {
        perror("stat(cache)");
        fprintf(stderr, "Creating cache directory at `%s`.\n", cache_dir);
        assert(mkdir(cache_dir, 0777) != -1);
    } else if (!S_ISDIR(dir.st_mode)) {
        fprintf(stderr, "Cache directory `$HOME/%s` is not a directory.\n", CACHE_DIR);
        return 1;
    }
    free(cache_dir);
    if (stat(build_dir, &dir) == -1) {
        perror("stat(build)");
        fprintf(stderr, "Creating build directory at `%s`.\n", build_dir);
        assert(mkdir(build_dir, 0777) != -1);
    } else if (!S_ISDIR(dir.st_mode)) {
        fprintf(stderr, "Build directory `$HOME/%s` is not a directory.\n", BUILD_DIR);
        return 1;
    }
    
    char *target = args->target;
    size_t target_len = strlen(target);
    size_t exec_len = target_len + prefix_len + build_dir_len;
    char *exec = malloc(exec_len + 1);
    assert(exec);
    snprintf(exec, exec_len + 1, "%s%s%s", build_dir, prefix, target);
    bool exec_exists = access(exec, F_OK) != -1;
    bool exec_needs_compile = !exec_exists || args->force;
    if (!exec_needs_compile) {
        struct stat exec_stat;
        struct stat target_stat;
        stat(exec, &exec_stat);
        stat(target, &target_stat);
        exec_needs_compile = exec_stat.st_mtime < target_stat.st_mtime;
    }
    if (exec_needs_compile) {
        fprintf(stderr, "Compiling `%s` at `%s`...\n", target, exec);
        size_t compile_len = strlen(COMPILER) + strlen(" -o ") + exec_len + strlen(" ") + target_len + strlen(" ") + cflags_len;
        char *compile = malloc(compile_len + 1);
        assert(compile);
        snprintf(compile, compile_len + 1, "%s -o %s %s %s", COMPILER, exec, target, cflags);
        system(compile);
        free(compile);
    }
    if (args->num_extra > 0) {
        size_t exec_total_len = exec_len;
        for (size_t i = 0; i < args->num_extra; i++) {
            exec_total_len += strlen(args->extra[i]) + 3;
        }
        char *exec_cmd = malloc(exec_total_len + 1);
        assert(exec_cmd);
        strcpy(exec_cmd, exec);
        for (size_t i = 0; i < args->num_extra; i++) {
            strcat(exec_cmd, " \"");
            strcat(exec_cmd, args->extra[i]);
            strcat(exec_cmd, "\"");
        }
        exec_cmd[exec_total_len] = '\0';
        system(exec_cmd);
        free(exec_cmd);
    } else {
        system(exec);
    }
    if (!cache) {
        fprintf(stderr, "Removing executable `%s`...\n", exec);
        remove(exec);
    }
    free(exec);
    size_t cmd_size = strlen("find  -type f -atime +1 -delete") + build_dir_len + 1;
    char *cmd = malloc(cmd_size);
    assert(cmd);
    snprintf(cmd, cmd_size, "find %s -type f -atime +1 -delete", build_dir);
    system(cmd);
}