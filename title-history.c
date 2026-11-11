#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Date { int year, month, day; };

static int date(const char *s, struct Date *d)
{
    char trailing;
    return sscanf(s, "%4d.%2d.%2d%c", &d->year, &d->month, &d->day, &trailing) == 3
        && d->year > 0 && d->month >= 1 && d->month <= 12
        && d->day >= 1 && d->day <= 31;
}

static int compare(struct Date a, struct Date b)
{
    if (a.year != b.year) return a.year - b.year;
    if (a.month != b.month) return a.month - b.month;
    return a.day - b.day;
}

enum { WORD = 256, QUOTED, INVALID };

static int token(FILE *input, char *word, size_t capacity)
{
    int ch;
    do {
        ch = fgetc(input);
        if (ch == '#') {
            do { ch = fgetc(input); } while (ch != '\n' && ch != EOF);
        }
    } while (isspace(ch));
    if (ch == EOF || strchr("{}=", ch)) return ch;

    size_t length = 0;
    int kind = WORD;
    if (ch == '"') {
        kind = QUOTED;
        while ((ch = fgetc(input)) != '"' && ch != EOF) {
            if (ch == '\\') ch = fgetc(input);
            if (ch == EOF || length + 1 == capacity) return INVALID;
            word[length++] = (char)ch;
        }
        if (ch == EOF) return INVALID;
    } else {
        do {
            if (length + 1 == capacity) return INVALID;
            word[length++] = (char)ch;
            ch = fgetc(input);
        } while (ch != EOF && !isspace(ch) && !strchr("{}=#\"", ch));
        if (ch != EOF && ungetc(ch, input) == EOF) return INVALID;
    }
    word[length] = '\0';
    return kind;
}

int main(int argc, char **argv)
{
    struct Date cutoff;
    if (argc != 3 || !date(argv[2], &cutoff)) {
        fprintf(stderr, "Usage: %s TITLE_HISTORY_FILE YEAR.MONTH.DAY\n", argv[0]);
        return EXIT_FAILURE;
    }
    FILE *input = fopen(argv[1], "rb");
    if (!input) {
        perror(argv[1]);
        return EXIT_FAILURE;
    }

    char word[4096], previous[4096] = "", holder[4096] = "";
    struct Date entry = {0}, latest = {0};
    int depth = 0, dated = 0, previous_kind = 0, before_previous = 0;
    int kind;
    while ((kind = token(input, word, sizeof word)) != EOF) {
        if (kind == INVALID) break;
        if (kind == '{') {
            if (depth == 0)
                dated = previous_kind == '=' && before_previous == WORD && date(previous, &entry);
            ++depth;
        } else if (kind == '}') {
            if (depth == 0) {
                kind = INVALID;
                break;
            }
            --depth;
        } else if (kind == WORD || kind == QUOTED) {
            if (depth == 1 && dated && previous_kind == '=' && before_previous == WORD
                && strcmp(previous, "holder") == 0 && compare(entry, cutoff) <= 0
                && compare(entry, latest) >= 0) {
                strcpy(holder, word);
                latest = entry;
            }
            strcpy(previous, word);
        }
        before_previous = previous_kind;
        previous_kind = kind;
    }

    int failed = ferror(input) || depth != 0 || kind == INVALID;
    fclose(input);
    if (failed) {
        fprintf(stderr, "Could not read title history: %s\n", argv[1]);
        return EXIT_FAILURE;
    }
    if (latest.year)
        printf("holder=%s effective=%d.%d.%d\n", holder, latest.year, latest.month, latest.day);
    else
        puts("no dated holder assignment");
    return EXIT_SUCCESS;
}
