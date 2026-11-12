#include "debug_console.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

bool debug_console_is_toggle_key(const SDL_KeyboardEvent *key)
{
    return !key->repeat &&
           (key->keysym.scancode == SDL_SCANCODE_GRAVE || key->keysym.sym == SDLK_BACKQUOTE);
}

void debug_console_toggle(struct debug_console *console)
{
    console->active = !console->active;
    if (console->active) {
        console->buffer[0] = '\0';
        console->length = 0;
        SDL_StartTextInput();
    } else {
        SDL_StopTextInput();
    }
}

void debug_console_text(struct debug_console *console, const char *text)
{
    for (const unsigned char *p = (const unsigned char *)text; *p; ++p) {
        /* Printable ASCII only. Ignore the toggle key itself: SDL sends its
           character as text right after the key press that opened the console. */
        if (*p < 32 || *p > 126 || *p == '`' || *p == '~')
            continue;
        if (console->length + 1 < sizeof console->buffer) {
            console->buffer[console->length++] = (char)*p;
            console->buffer[console->length] = '\0';
        }
    }
}

bool debug_console_key(struct debug_console *console, SDL_Keycode key,
                       char *command, size_t command_size)
{
    switch (key) {
    case SDLK_BACKSPACE:
        if (console->length > 0)
            console->buffer[--console->length] = '\0';
        break;
    case SDLK_ESCAPE:
        debug_console_toggle(console);
        break;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
        if (console->length == 0)
            break;
        snprintf(command, command_size, "%s", console->buffer);
        console->buffer[0] = '\0';
        console->length = 0;
        return true;
    }
    return false;
}

void debug_console_print(struct debug_console *console, const char *format, ...)
{
    if (console->log_count == DEBUG_LOG_LINES) {
        memmove(console->log[0], console->log[1], sizeof console->log[0] * (DEBUG_LOG_LINES - 1));
        --console->log_count;
    }
    va_list args;
    va_start(args, format);
    vsnprintf(console->log[console->log_count++], DEBUG_LOG_WIDTH, format, args);
    va_end(args);
}

void debug_console_clear(struct debug_console *console)
{
    console->log_count = 0;
}
