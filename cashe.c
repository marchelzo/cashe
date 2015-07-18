#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdnoreturn.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>

#define BUFFER_SIZE 4096ULL

static FILE *f;
static char buffer[BUFFER_SIZE];
static char const *pname;
static char **args;
static size_t length;
static size_t white;
static bool use_previous_input;
static size_t indent_amount;
static FILE *shell;
static char *shell_path;

struct expression {
    enum { RULE, COMMAND } type;
    union {
        char *command;
        struct {
            char *pattern;
            struct expression **children;
        };
    };
};

noreturn static void oom(void)
{
    fprintf(stderr, "%s: out of memory\n", pname);
    exit(EXIT_FAILURE);
}

noreturn static void too_long(void)
{
    fprintf(stderr, "%s: input line was too long: max length is %llu\n", pname, BUFFER_SIZE - 2);
    exit(EXIT_FAILURE);
}

noreturn static void invalid_space(void)
{
    fprintf(stderr, "%s: invalid space: only use ` ` for indentation\n", pname);
    exit(EXIT_FAILURE);
}

static bool blank(char const *s)
{
    while (*s)
        if (!isspace(*s++))
            return false;

    return true;
}

static bool match(char const *pattern)
{
    if (!*args)
        return !strcmp(pattern, "_");

    if (!strcmp(pattern, *args)) {
        args += 1;
        return true;
    }

    return false;
}

static size_t next_line(void)
{
    do {
        buffer[BUFFER_SIZE - 1] = 'x';
        if (!fgets(buffer, BUFFER_SIZE, f))
            return 0;
        if (buffer[BUFFER_SIZE - 1] != 'x')
            too_long();

    } while (blank(buffer));

    return strlen(buffer);
}

static size_t leading_whitespace(char const *s)
{
    size_t result = 0;

    while (isspace(*s)) {
        if (*s++ == ' ')
            result += 1;
        else
            invalid_space();
    }

    return result;
}

static struct expression *parse_expression(size_t indent)
{
    if (!use_previous_input) {
        length = next_line();
        white = leading_whitespace(buffer);
    }

    if (!length || white < indent) {
        use_previous_input = true;
        return NULL;
    } else {
        use_previous_input = false;
    }

    struct expression *e = malloc(sizeof *e);

    if (!e) oom();

    if (length > 1 && buffer[length - 2] == ':') {
        e->type = RULE;
        e->pattern = malloc(length - white - 1);
        if (!e->pattern) oom();
        strncpy(e->pattern, buffer + white, length - white - 2);
        e->pattern[length - white - 2] = 0;

        size_t alloc = 0;
        size_t n = 0;
        e->children = NULL;

        do {
            if (n == alloc) {
                alloc = alloc ? alloc * 2 : 1;
                e->children = realloc(e->children, alloc * sizeof *e->children);
                if (!e->children) oom();
            }

            e->children[n] = parse_expression(indent + indent_amount);
        } while (e->children[n++]);

    } else {
        e->type = COMMAND;
        e->command = malloc(length);
        if (!e->command) oom();
        strcpy(e->command, buffer + white);
    }

    return e;
}


static struct expression **parse_program(void)
{
    size_t alloc = 0;
    size_t n = 0;
    struct expression **program = NULL;

    do {
        if (n == alloc) {
            alloc = alloc ? alloc * 2 : 1;
            program = realloc(program, alloc * sizeof *program);
            if (!program) oom();
        }

        program[n] = parse_expression(0);

    } while (program[n++]);

    return program;
}

static void write_to_shell(char const *s)
{
    bool in_string = false;
    bool in_dstring = false;

    while (*s) {
        if (*s == '\\') {
            fputc(*s, shell);
            s += 1;
            if (*s) {
                fputc(*s, shell);
                s += 1;
            }
        } else if (*s == '\'') {
            in_string -= 1;
            fputc('\'', shell);
            s += 1;
        } else if (!in_string && *s == '"') {
            in_dstring -= 1;
            fputc('"', shell);
            s += 1;
        } else if (!in_string && *s == '$' && !strncmp("ARGS", s + 1, 4) && !isalnum(s[5])) {
            s += 5;
            if (in_dstring)
                for (char **arg = args; arg[0]; ++arg)
                    fprintf(shell, "%s%s", arg[0], arg[1] ? " " : "");
            else
                for (char **arg = args; arg[0]; ++arg)
                    fprintf(shell, "'%s'%s", arg[0], arg[1] ? " " : "");
        } else {
            fputc(*s, shell);
            s += 1;
        }
    }
}

static void execute(struct expression **program)
{
    while (program[0]) {
        if (program[0]->type == COMMAND) {
            write_to_shell(program[0]->command);
        } else if (match(program[0]->pattern)) {
            execute(program[0]->children);
            return;
        }

        program += 1;
    }
}

int main(int argc, char *argv[])
{
    pname = argv[0];

    args = argv + 2;

    char *end;
    long long value = strtoll(argv[1], &end, 10);

    if (value < 1 || errno || *end || end == argv[1]) {
        fprintf(stderr, "%s: invalid indentation amount: `%s`\n", pname, argv[1]);
        return 1;
    }

    indent_amount = value;

    if (isatty(fileno(stdin))) {
        args += 1;
        f = fopen(argv[2], "r");
        if (!f) {
            fprintf(stderr, "%s: failed to open file for reading: %s\n", pname, argv[1]);
            return 1;
        }
    } else {
        f = stdin;
    }

    struct expression **program = parse_program();

    if (!program) {
        fprintf(stderr, "%s: failed to parse .iconfig file\n", pname);
        return 1;
    }

    shell_path = getenv("SHELL");

    if (!shell_path)
        shell_path = "/bin/sh";

    shell = popen(shell_path, "w");

    if (!shell) {
        fprintf(stderr, "%s: failed to spawn shell: %s\n", pname, shell_path);
        return 1;
    }

    execute(program);

    fputs("\nexit\n", shell);

    pclose(shell);

    return 0;
}
