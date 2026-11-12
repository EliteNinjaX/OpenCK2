#ifndef OPENCK2_DEBUG_CONSOLE_H
#define OPENCK2_DEBUG_CONSOLE_H

#include <SDL.h>
#include <stdbool.h>

enum { DEBUG_BUFFER_SIZE = 128, DEBUG_LOG_LINES = 8, DEBUG_LOG_WIDTH = 160 };

struct debug_console {
    char buffer[DEBUG_BUFFER_SIZE];
    size_t length;
    char log[DEBUG_LOG_LINES][DEBUG_LOG_WIDTH];
    int log_count;
    bool active;
};

/* The ` / ~ key (by physical position, so it works on any keyboard layout). */
bool debug_console_is_toggle_key(const SDL_KeyboardEvent *key);
void debug_console_toggle(struct debug_console *console);

/* Feed SDL_TEXTINPUT text into the input line. */
void debug_console_text(struct debug_console *console, const char *text);

/* Handle a key press while the console is open. Returns true when Enter submitted a
   non-empty command; the command is copied into `command` and the input line is cleared. */
bool debug_console_key(struct debug_console *console, SDL_Keycode key,
                       char *command, size_t command_size);

/* Appends a line to the console output, scrolling older lines up. */
void debug_console_print(struct debug_console *console, const char *format, ...);
void debug_console_clear(struct debug_console *console);

#endif
