#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CK2_STEAM_APP_ID "203770"

static void print_usage(FILE *stream)
{
    fputs("Usage: ck2-launcher [--help]\n"
          "Launch the local Steam installation of Crusader Kings II.\n",
          stream);
}

static int launch_ck2(void)
{
    execlp("steam", "steam", "-applaunch", CK2_STEAM_APP_ID, (char *)NULL);

    const int error = errno;
    fprintf(stderr, "ck2-launcher: could not start Steam: %s\n", strerror(error));
    if (error == ENOENT)
        fputs("Make sure the Steam client is available on PATH.\n", stderr);

    return EXIT_FAILURE;
}

int main(int argc, char *argv[])
{
    if (argc == 1)
        return launch_ck2();

    if (argc == 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
        print_usage(stdout);
        return EXIT_SUCCESS;
    }

    print_usage(stderr);
    return EXIT_FAILURE;
}
