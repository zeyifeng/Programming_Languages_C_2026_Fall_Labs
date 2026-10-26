// Zeyi Feng 241ADB024
//
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
// Resources used: user-provided lab specification and calc-tests.zip.

#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define STUDENT_FIRST "zeyi"
#define STUDENT_LAST "feng"
#define STUDENT_ID "241ADB024"

/* A decimal point marks a floating operand. Each operator promotes both sides
 * only when at least one operand is floating; otherwise it keeps integer math. */
typedef struct {
    long long integer;
    double real;
    int is_float;
} Value;

typedef struct {
    char *text;
    size_t position;
    size_t end;
    size_t error_position;
} Parser;

typedef struct {
    char **items;
    size_t count;
    size_t capacity;
} StringList;

static char *join_path(const char *directory, const char *name);

static Value make_integer(long long number)
{
    Value value;
    value.integer = number;
    value.real = (double)number;
    value.is_float = 0;
    return value;
}

static Value make_float(double number)
{
    Value value;
    value.integer = 0;
    value.real = number;
    value.is_float = 1;
    return value;
}

static void set_error(Parser *parser, size_t buffer_position)
{
    if (parser->error_position == 0) {
        /* Positions are indexes in the original whole-file buffer, so +1 gives
         * the required one-based byte offset, including comments and newlines. */
        parser->error_position = buffer_position + 1;
    }
}

static void skip_space(Parser *parser)
{
    while (parser->position < parser->end &&
           isspace((unsigned char)parser->text[parser->position])) {
        parser->position++;
    }
}

static size_t next_token_position(Parser *parser)
{
    skip_space(parser);
    return parser->position;
}

static int checked_add(long long left, long long right, long long *result)
{
    if ((right > 0 && left > LLONG_MAX - right) ||
        (right < 0 && left < LLONG_MIN - right)) {
        return 0;
    }
    *result = left + right;
    return 1;
}

static int checked_subtract(long long left, long long right, long long *result)
{
    if ((right < 0 && left > LLONG_MAX + right) ||
        (right > 0 && left < LLONG_MIN + right)) {
        return 0;
    }
    *result = left - right;
    return 1;
}

static unsigned long long magnitude(long long value)
{
    if (value < 0) {
        return 0ULL - (unsigned long long)value;
    }
    return (unsigned long long)value;
}

static int checked_multiply(long long left, long long right, long long *result)
{
    int negative = (left < 0) != (right < 0);
    unsigned long long a = magnitude(left);
    unsigned long long b = magnitude(right);
    unsigned long long limit = (unsigned long long)LLONG_MAX;
    unsigned long long product;

    if (negative) {
        limit++;
    }
    if (a != 0 && b > limit / a) {
        return 0;
    }
    product = a * b;
    if (negative) {
        if (product == (unsigned long long)LLONG_MAX + 1ULL) {
            *result = LLONG_MIN;
        } else {
            *result = -(long long)product;
        }
    } else {
        *result = (long long)product;
    }
    return 1;
}

static int checked_integer_power(long long base, long long exponent,
                                 long long *result)
{
    unsigned long long remaining = (unsigned long long)exponent;
    long long product = 1;
    long long factor = base;

    while (remaining != 0) {
        if ((remaining & 1ULL) != 0 &&
            !checked_multiply(product, factor, &product)) {
            return 0;
        }
        remaining >>= 1;
        if (remaining != 0 &&
            !checked_multiply(factor, factor, &factor)) {
            return 0;
        }
    }
    *result = product;
    return 1;
}

static Value parse_expression(Parser *parser);

/* Numbers are scanned here so a decimal must contain digits on both sides of
 * its dot; temporarily terminating the already-read buffer lets strtod convert
 * just that token without copying a potentially long line. */
static Value parse_primary(Parser *parser)
{
    Value value = make_integer(0);
    size_t start;
    size_t dot_position;
    size_t token_end;
    unsigned long long integer_value;
    char *conversion_end;
    char saved;

    skip_space(parser);
    if (parser->position >= parser->end) {
        /* A missing operand is reported at the line's newline or EOF+1. */
        set_error(parser, parser->end);
        return value;
    }

    if (parser->text[parser->position] == '(') {
        parser->position++;
        value = parse_expression(parser);
        if (parser->error_position != 0) {
            return value;
        }
        skip_space(parser);
        if (parser->position >= parser->end) {
            /* The opening parenthesis is incomplete at this same line boundary. */
            set_error(parser, parser->end);
        } else if (parser->text[parser->position] == ')') {
            parser->position++;
        } else {
            set_error(parser, parser->position);
        }
        return value;
    }

    if (!isdigit((unsigned char)parser->text[parser->position])) {
        /* No primary expression begins with this token, so its own byte is bad. */
        set_error(parser, parser->position);
        return value;
    }

    start = parser->position;
    while (parser->position < parser->end &&
           isdigit((unsigned char)parser->text[parser->position])) {
        parser->position++;
    }

    if (parser->position < parser->end &&
        parser->text[parser->position] == '.') {
        dot_position = parser->position;
        parser->position++;
        if (parser->position >= parser->end ||
            !isdigit((unsigned char)parser->text[parser->position])) {
            set_error(parser, dot_position);
            return value;
        }
        while (parser->position < parser->end &&
               isdigit((unsigned char)parser->text[parser->position])) {
            parser->position++;
        }

        token_end = parser->position;
        saved = parser->text[token_end];
        parser->text[token_end] = '\0';
        value = make_float(strtod(parser->text + start, &conversion_end));
        parser->text[token_end] = saved;
        if (conversion_end != parser->text + token_end || !isfinite(value.real)) {
            set_error(parser, start);
        }
    } else {
        size_t digit_position;
        integer_value = 0;
        for (digit_position = start; digit_position < parser->position;
             digit_position++) {
            unsigned int digit =
                (unsigned int)(parser->text[digit_position] - '0');
            if (integer_value > ((unsigned long long)LLONG_MAX - digit) / 10ULL) {
                set_error(parser, start);
                return value;
            }
            integer_value = integer_value * 10ULL + digit;
        }
        value = make_integer((long long)integer_value);
    }
    return value;
}

/* One function per grammar level makes * and / bind more tightly than + and -.
 * power() calls itself on the right, which groups 2 ** 3 ** 2 as 2 ** (3 ** 2). */
static Value parse_power(Parser *parser)
{
    Value left = parse_primary(parser);
    size_t operator_position;
    size_t exponent_position;
    Value right;

    if (parser->error_position != 0) {
        return left;
    }
    skip_space(parser);
    if (parser->position + 1 >= parser->end ||
        parser->text[parser->position] != '*' ||
        parser->text[parser->position + 1] != '*') {
        return left;
    }

    operator_position = parser->position;
    parser->position += 2;
    exponent_position = next_token_position(parser);
    right = parse_power(parser);
    if (parser->error_position != 0) {
        return left;
    }

    if (!left.is_float && !right.is_float) {
        long long answer;
        if (right.integer < 0) {
            /* An integer exponent cannot be negative; point to its first token,
             * even when parentheses are what produce the negative value. */
            set_error(parser, exponent_position);
            return left;
        }
        if (!checked_integer_power(left.integer, right.integer, &answer)) {
            set_error(parser, operator_position);
            return left;
        }
        return make_integer(answer);
    }

    {
        double answer = pow(left.is_float ? left.real : (double)left.integer,
                            right.is_float ? right.real : (double)right.integer);
        if (!isfinite(answer)) {
            /* Float overflow and NaN are attributed to the operation that made them. */
            set_error(parser, operator_position);
            return left;
        }
        return make_float(answer);
    }
}

static Value parse_term(Parser *parser)
{
    Value left = parse_power(parser);

    while (parser->error_position == 0) {
        size_t operator_position;
        size_t right_position;
        char operation;
        Value right;

        skip_space(parser);
        if (parser->position >= parser->end ||
            (parser->text[parser->position] != '*' &&
             parser->text[parser->position] != '/')) {
            break;
        }
        if (parser->text[parser->position] == '*' &&
            parser->position + 1 < parser->end &&
            parser->text[parser->position + 1] == '*') {
            break;
        }

        operator_position = parser->position;
        operation = parser->text[parser->position++];
        right_position = next_token_position(parser);
        right = parse_power(parser);
        if (parser->error_position != 0) {
            break;
        }

        if (operation == '/' &&
            (right.is_float ? right.real == 0.0 : right.integer == 0)) {
            /* Division errors identify the start of the entire right operand. */
            set_error(parser, right_position);
            break;
        }

        if (!left.is_float && !right.is_float) {
            long long answer;
            int ok = 1;
            if (operation == '*') {
                ok = checked_multiply(left.integer, right.integer, &answer);
            } else if (left.integer == LLONG_MIN && right.integer == -1) {
                ok = 0;
            } else {
                answer = left.integer / right.integer;
            }
            if (!ok) {
                set_error(parser, operator_position);
                break;
            }
            left = make_integer(answer);
        } else {
            double a = left.is_float ? left.real : (double)left.integer;
            double b = right.is_float ? right.real : (double)right.integer;
            double answer = operation == '*' ? a * b : a / b;
            if (!isfinite(answer)) {
                set_error(parser, operator_position);
                break;
            }
            left = make_float(answer);
        }
    }
    return left;
}

static Value parse_expression(Parser *parser)
{
    Value left = parse_term(parser);

    while (parser->error_position == 0) {
        size_t operator_position;
        char operation;
        Value right;

        skip_space(parser);
        if (parser->position >= parser->end ||
            (parser->text[parser->position] != '+' &&
             parser->text[parser->position] != '-')) {
            break;
        }

        operator_position = parser->position;
        operation = parser->text[parser->position++];
        right = parse_term(parser);
        if (parser->error_position != 0) {
            break;
        }

        if (!left.is_float && !right.is_float) {
            long long answer;
            int ok = operation == '+'
                         ? checked_add(left.integer, right.integer, &answer)
                         : checked_subtract(left.integer, right.integer, &answer);
            if (!ok) {
                set_error(parser, operator_position);
                break;
            }
            left = make_integer(answer);
        } else {
            double a = left.is_float ? left.real : (double)left.integer;
            double b = right.is_float ? right.real : (double)right.integer;
            double answer = operation == '+' ? a + b : a - b;
            if (!isfinite(answer)) {
                set_error(parser, operator_position);
                break;
            }
            left = make_float(answer);
        }
    }
    return left;
}

static char *read_entire_file(const char *path, size_t *length)
{
    FILE *input = fopen(path, "rb");
    char *buffer;
    size_t capacity = 65536;

    if (input == NULL) {
        return NULL;
    }
    buffer = (char *)malloc(capacity + 1);
    if (buffer == NULL) {
        fclose(input);
        errno = ENOMEM;
        return NULL;
    }

    *length = 0;
    for (;;) {
        size_t amount;
        if (*length == capacity) {
            size_t new_capacity;
            char *grown;
            if (capacity > (size_t)-1 / 2) {
                free(buffer);
                fclose(input);
                errno = ENOMEM;
                return NULL;
            }
            new_capacity = capacity * 2;
            grown = (char *)realloc(buffer, new_capacity + 1);
            if (grown == NULL) {
                free(buffer);
                fclose(input);
                errno = ENOMEM;
                return NULL;
            }
            buffer = grown;
            capacity = new_capacity;
        }
        amount = fread(buffer + *length, 1, capacity - *length, input);
        *length += amount;
        if (amount == 0) {
            if (ferror(input)) {
                int saved_errno = errno != 0 ? errno : EIO;
                free(buffer);
                fclose(input);
                errno = saved_errno;
                return NULL;
            }
            break;
        }
    }
    if (fclose(input) != 0) {
        int saved_errno = errno;
        free(buffer);
        errno = saved_errno;
        return NULL;
    }
    buffer[*length] = '\0';
    return buffer;
}

/* Geometric buffer growth keeps large files linear-time without recopies on
 * every read. The temporary stream keeps partial answers out of the real file: if a
 * later line fails, the final file contains only that first ERROR position. */
static int evaluate_file(const char *input_path, const char *output_dir,
                         const char *input_base)
{
    size_t length = 0;
    char *text = read_entire_file(input_path, &length);
    FILE *staged;
    size_t line_start = 0;
    size_t error_position = 0;
    char *file_name;
    char *output_path;
    size_t name_length;
    int status = 0;

    if (text == NULL) {
        fprintf(stderr, "calc: cannot read '%s': %s\n", input_path,
                strerror(errno));
        return 0;
    }
    staged = tmpfile();
    if (staged == NULL) {
        fprintf(stderr, "calc: cannot create temporary output: %s\n",
                strerror(errno));
        free(text);
        return 0;
    }

    while (line_start < length) {
        size_t line_end = line_start;
        size_t first = line_start;
        Parser parser;
        Value value;
        int has_newline;

        while (line_end < length && text[line_end] != '\n') {
            line_end++;
        }
        has_newline = line_end < length;
        while (first < line_end && isspace((unsigned char)text[first])) {
            first++;
        }

        if (first < line_end && text[first] != '#') {
            parser.text = text;
            parser.position = line_start;
            parser.end = line_end;
            parser.error_position = 0;
            /* Parsing uses absolute indexes into the input buffer; end is the
             * line boundary so no expression can silently continue next line. */
            value = parse_expression(&parser);
            if (parser.error_position == 0) {
                skip_space(&parser);
                if (parser.position < parser.end) {
                    set_error(&parser, parser.position);
                }
            }
            if (parser.error_position != 0) {
                error_position = parser.error_position;
                break;
            }
            if (value.is_float) {
                if (fprintf(staged, "%.15g\n", value.real) < 0) {
                    status = 0;
                    break;
                }
            } else if (fprintf(staged, "%lld\n", value.integer) < 0) {
                status = 0;
                break;
            }
        }

        line_start = has_newline ? line_end + 1 : length;
    }

    if (ferror(staged)) {
        fprintf(stderr, "calc: cannot stage output for '%s'\n", input_path);
        fclose(staged);
        free(text);
        return 0;
    }

    name_length = strlen(input_base) + strlen(STUDENT_FIRST) +
                  strlen(STUDENT_LAST) + strlen(STUDENT_ID) + 8;
    file_name = (char *)malloc(name_length);
    if (file_name == NULL) {
        fprintf(stderr, "calc: out of memory while naming output\n");
        fclose(staged);
        free(text);
        return 0;
    }
    (void)snprintf(file_name, name_length, "%s_%s_%s_%s.txt", input_base,
                   STUDENT_FIRST, STUDENT_LAST, STUDENT_ID);
    output_path = join_path(output_dir, file_name);
    free(file_name);
    if (output_path == NULL) {
        fprintf(stderr, "calc: out of memory while naming output\n");
        fclose(staged);
        free(text);
        return 0;
    }

    {
        FILE *output = fopen(output_path, "wb");
        if (output == NULL) {
            fprintf(stderr, "calc: cannot write '%s': %s\n", output_path,
                    strerror(errno));
            status = 0;
        } else {
            if (error_position != 0) {
                if (fprintf(output, "ERROR:%zu\n", error_position) < 0) {
                    status = 0;
                } else {
                    status = 1;
                }
            } else if (fflush(staged) != 0 || fseek(staged, 0, SEEK_SET) != 0) {
                fprintf(stderr, "calc: cannot read staged output\n");
                status = 0;
            } else {
                char copy_buffer[8192];
                size_t amount;
                status = 1;
                while ((amount = fread(copy_buffer, 1, sizeof copy_buffer,
                                       staged)) != 0) {
                    if (fwrite(copy_buffer, 1, amount, output) != amount) {
                        status = 0;
                        break;
                    }
                }
                if (ferror(staged)) {
                    status = 0;
                }
            }
            if (fclose(output) != 0) {
                status = 0;
            }
            if (!status) {
                fprintf(stderr, "calc: failed while writing '%s'\n", output_path);
            }
        }
    }

    free(output_path);
    fclose(staged);
    free(text);
    return status;
}

static char *duplicate_string(const char *text)
{
    size_t length = strlen(text) + 1;
    char *copy = (char *)malloc(length);
    if (copy != NULL) {
        memcpy(copy, text, length);
    }
    return copy;
}

static char *join_path(const char *directory, const char *name)
{
    size_t directory_length = strlen(directory);
    size_t name_length = strlen(name);
    int needs_separator = directory_length != 0 &&
                          directory[directory_length - 1] != '/' &&
                          directory[directory_length - 1] != '\\';
    char *joined = (char *)malloc(directory_length + (size_t)needs_separator +
                                  name_length + 1);
    if (joined == NULL) {
        return NULL;
    }
    memcpy(joined, directory, directory_length);
    if (needs_separator) {
        joined[directory_length++] = '/';
    }
    memcpy(joined + directory_length, name, name_length + 1);
    return joined;
}

static char *input_base_name(const char *path, int strip_txt)
{
    size_t path_length = strlen(path);
    size_t start = 0;
    size_t index;
    size_t length;
    const char *base;
    char *result;
    while (path_length > 1 &&
           (path[path_length - 1] == '/' || path[path_length - 1] == '\\')) {
        path_length--;
    }
    for (index = 0; index < path_length; index++) {
        if (path[index] == '/' || path[index] == '\\') {
            start = index + 1;
        }
    }
    base = path + start;
    length = path_length - start;
    if (length == 0) {
        base = "root";
        length = 4;
    }
    if (strip_txt && length >= 4 && strcmp(base + length - 4, ".txt") == 0) {
        length -= 4;
    }
    result = (char *)malloc(length + 1);
    if (result != NULL) {
        memcpy(result, base, length);
        result[length] = '\0';
    }
    return result;
}

static int ensure_directory(const char *path)
{
    struct stat information;
    if (stat(path, &information) == 0) {
        if (S_ISDIR(information.st_mode)) {
            return 1;
        }
        fprintf(stderr, "calc: output path '%s' is not a directory\n", path);
        return 0;
    }
    if (errno != ENOENT) {
        fprintf(stderr, "calc: cannot inspect output directory '%s': %s\n",
                path, strerror(errno));
        return 0;
    }
    if (mkdir(path, 0777) == 0) {
        return 1;
    }
    if (errno == EEXIST && stat(path, &information) == 0 &&
        S_ISDIR(information.st_mode)) {
        return 1;
    }
    fprintf(stderr, "calc: cannot create output directory '%s': %s\n", path,
            strerror(errno));
    return 0;
}

static int append_string(StringList *list, const char *text)
{
    if (list->count == list->capacity) {
        size_t new_capacity = list->capacity == 0 ? 16 : list->capacity * 2;
        char **grown;
        if (new_capacity < list->capacity ||
            new_capacity > (size_t)-1 / sizeof *list->items) {
            return 0;
        }
        grown = (char **)realloc(list->items, new_capacity * sizeof *list->items);
        if (grown == NULL) {
            return 0;
        }
        list->items = grown;
        list->capacity = new_capacity;
    }
    list->items[list->count] = duplicate_string(text);
    if (list->items[list->count] == NULL) {
        return 0;
    }
    list->count++;
    return 1;
}

static int compare_strings(const void *left, const void *right)
{
    const char *const *a = (const char *const *)left;
    const char *const *b = (const char *const *)right;
    return strcmp(*a, *b);
}

static int has_txt_suffix(const char *name)
{
    size_t length = strlen(name);
    return length >= 4 && strcmp(name + length - 4, ".txt") == 0;
}

static void free_string_list(StringList *list)
{
    size_t index;
    for (index = 0; index < list->count; index++) {
        free(list->items[index]);
    }
    free(list->items);
}

static int evaluate_directory(const char *input_dir, const char *output_dir)
{
    DIR *directory = opendir(input_dir);
    struct dirent *entry;
    StringList files = {NULL, 0, 0};
    size_t index;
    int ok = 1;

    if (directory == NULL) {
        fprintf(stderr, "calc: cannot open input directory '%s': %s\n",
                input_dir, strerror(errno));
        return 0;
    }
    errno = 0;
    while ((entry = readdir(directory)) != NULL) {
        char *candidate;
        struct stat information;
        if (!has_txt_suffix(entry->d_name)) {
            continue;
        }
        candidate = join_path(input_dir, entry->d_name);
        if (candidate == NULL) {
            fprintf(stderr, "calc: out of memory while listing input files\n");
            ok = 0;
            break;
        }
        if (stat(candidate, &information) == 0 && S_ISREG(information.st_mode)) {
            if (!append_string(&files, entry->d_name)) {
                fprintf(stderr, "calc: out of memory while listing input files\n");
                ok = 0;
                free(candidate);
                break;
            }
        }
        free(candidate);
        errno = 0;
    }
    if (errno != 0) {
        fprintf(stderr, "calc: error while reading input directory '%s': %s\n",
                input_dir, strerror(errno));
        ok = 0;
    }
    if (closedir(directory) != 0) {
        fprintf(stderr, "calc: error while closing input directory '%s'\n",
                input_dir);
        ok = 0;
    }
    if (ok) {
        if (files.count > 1) {
            qsort(files.items, files.count, sizeof *files.items, compare_strings);
        }
        for (index = 0; index < files.count; index++) {
            char *input_path = join_path(input_dir, files.items[index]);
            char *base = input_base_name(files.items[index], 1);
            if (input_path == NULL || base == NULL) {
                fprintf(stderr, "calc: out of memory while naming input\n");
                ok = 0;
            } else if (!evaluate_file(input_path, output_dir, base)) {
                ok = 0;
            }
            free(input_path);
            free(base);
            if (!ok) {
                break;
            }
        }
    }
    free_string_list(&files);
    return ok;
}

static void usage(void)
{
    fprintf(stderr,
            "Usage: calc [-d DIR | --dir DIR] [-o OUTDIR | --output-dir OUTDIR] [input.txt]\n");
}

int main(int argc, char **argv)
{
    const char *input_file = NULL;
    const char *input_dir = NULL;
    const char *output_option = NULL;
    char *default_dir = NULL;
    char *directory_base = NULL;
    const char *output_dir;
    int index;
    int ok;

    for (index = 1; index < argc; index++) {
        if (strcmp(argv[index], "-d") == 0 ||
            strcmp(argv[index], "--dir") == 0) {
            if (input_dir != NULL || index + 1 >= argc || argv[index + 1][0] == '-') {
                usage();
                return 2;
            }
            input_dir = argv[++index];
        } else if (strcmp(argv[index], "-o") == 0 ||
                   strcmp(argv[index], "--output-dir") == 0) {
            if (output_option != NULL || index + 1 >= argc || argv[index + 1][0] == '-') {
                usage();
                return 2;
            }
            output_option = argv[++index];
        } else if (argv[index][0] == '-') {
            usage();
            return 2;
        } else if (input_file == NULL) {
            input_file = argv[index];
        } else {
            usage();
            return 2;
        }
    }

    if ((input_dir == NULL && input_file == NULL) ||
        (input_dir != NULL && input_file != NULL)) {
        usage();
        return 2;
    }

    if (output_option != NULL) {
        output_dir = output_option;
    } else {
        char *base = input_dir != NULL ? input_base_name(input_dir, 0)
                                       : input_base_name(input_file, 1);
        size_t needed;
        if (base == NULL) {
            fprintf(stderr, "calc: out of memory while naming output directory\n");
            return 1;
        }
        needed = strlen(base) + strlen(STUDENT_FIRST) + strlen(STUDENT_ID) + 3;
        default_dir = (char *)malloc(needed);
        if (default_dir != NULL) {
            (void)snprintf(default_dir, needed, "%s_%s_%s", base,
                           STUDENT_FIRST, STUDENT_ID);
        }
        free(base);
        if (default_dir == NULL) {
            fprintf(stderr, "calc: out of memory while naming output directory\n");
            return 1;
        }
        output_dir = default_dir;
    }

    if (!ensure_directory(output_dir)) {
        free(default_dir);
        return 1;
    }

    if (input_dir != NULL) {
        ok = evaluate_directory(input_dir, output_dir);
    } else {
        directory_base = input_base_name(input_file, 1);
        if (directory_base == NULL) {
            fprintf(stderr, "calc: out of memory while naming input\n");
            free(default_dir);
            return 1;
        }
        ok = evaluate_file(input_file, output_dir, directory_base);
        free(directory_base);
    }
    free(default_dir);
    return ok ? 0 : 1;
}
