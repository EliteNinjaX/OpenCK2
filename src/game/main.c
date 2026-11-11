#include "assets.h"
#include "music.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { WIDTH = 1600, HEIGHT = 900, BUTTON_WIDTH = 232, BUTTON_HEIGHT = 52,
       CREDIT_LINE = 24 };

enum screen { MAIN_MENU, OPTIONS, CREDITS };
enum action { UNAVAILABLE, OPEN_OPTIONS, OPEN_CREDITS, QUIT, TOGGLE_FULLSCREEN, TOGGLE_MUSIC, BACK };

struct button {
    SDL_Rect area;
    const char *label;
    enum action action;
};

static const struct button menu_buttons[] = {
    {{684, 311, BUTTON_WIDTH, BUTTON_HEIGHT}, "Single Player", UNAVAILABLE},
    {{684, 361, BUTTON_WIDTH, BUTTON_HEIGHT}, "Multiplayer", UNAVAILABLE},
    {{684, 411, BUTTON_WIDTH, BUTTON_HEIGHT}, "Options", OPEN_OPTIONS},
    {{684, 461, BUTTON_WIDTH, BUTTON_HEIGHT}, "Credits", OPEN_CREDITS},
    {{684, 511, BUTTON_WIDTH, BUTTON_HEIGHT}, "Quit", QUIT},
};
static const struct button options_buttons[] = {
    {{684, 370, BUTTON_WIDTH, BUTTON_HEIGHT}, "Fullscreen: Off", TOGGLE_FULLSCREEN},
    {{684, 425, BUTTON_WIDTH, BUTTON_HEIGHT}, "Music: On", TOGGLE_MUSIC},
    {{684, 480, BUTTON_WIDTH, BUTTON_HEIGHT}, "Back", BACK},
};
static const struct button credits_buttons[] = {
    {{684, 720, BUTTON_WIDTH, BUTTON_HEIGHT}, "Back", BACK},
};

struct menu {
    struct menu_music music;
    enum screen screen;
    enum action focus, hover;
    int credit_scroll, credit_scroll_max;
    bool running;
};

static void blit(SDL_Renderer *renderer, SDL_Texture *texture, int x, int y)
{
    SDL_Rect target = {x, y, 0, 0};
    SDL_QueryTexture(texture, NULL, NULL, &target.w, &target.h);
    SDL_RenderCopy(renderer, texture, NULL, &target);
}

static void centered_text(SDL_Renderer *renderer, const struct menu_assets *assets,
                          const char *text, size_t length, int center, int y)
{
    const unsigned char *chars = (const unsigned char *)text;
    int width = 0;
    for (size_t i = 0; i < length; ++i)
        width += assets->glyphs[chars[i]].advance;

    int x = center - width / 2;
    for (size_t i = 0; i < length; ++i) {
        const struct glyph *glyph = &assets->glyphs[chars[i]];
        if (glyph->area.w > 0 && glyph->area.h > 0) {
            const SDL_Rect target = {x + glyph->x_offset, y + glyph->y_offset,
                                     glyph->area.w, glyph->area.h};
            SDL_RenderCopy(renderer, assets->font, &glyph->area, &target);
        }
        x += glyph->advance;
    }
}

static void draw_credits(SDL_Renderer *renderer, const struct menu_assets *assets, int offset)
{
    const SDL_Rect clip = {568, 165, 464, 535};
    SDL_RenderSetClipRect(renderer, &clip);
    const char *line = assets->credits;
    int y = clip.y - offset;

    while (*line && y < clip.y + clip.h) {
        const char *end = strchr(line, '\n');
        size_t length = end ? (size_t)(end - line) : strlen(line);
        if (length && line[length - 1] == '\r')
            --length;
        if (length && (unsigned char)*line == 0xa7) {
            ++line;
            --length;
        }
        if (y + CREDIT_LINE > clip.y)
            centered_text(renderer, assets, line, length, WIDTH / 2, y);
        if (!end)
            break;
        line = end + 1;
        y += CREDIT_LINE;
    }
    SDL_RenderSetClipRect(renderer, NULL);
}

static const struct button *buttons_for(enum screen screen, size_t *count)
{
    if (screen == OPTIONS) {
        *count = SDL_arraysize(options_buttons);
        return options_buttons;
    }
    if (screen == CREDITS) {
        *count = SDL_arraysize(credits_buttons);
        return credits_buttons;
    }
    *count = SDL_arraysize(menu_buttons);
    return menu_buttons;
}

static bool fullscreen(SDL_Window *window)
{
    return (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) != 0;
}

static bool action_enabled(const struct menu *menu, enum action action)
{
    return action != UNAVAILABLE && (action != TOGGLE_MUSIC || menu->music.track);
}

static const char *button_label(const struct button *button, SDL_Window *window,
                                const struct menu *menu)
{
    if (button->action == TOGGLE_FULLSCREEN && fullscreen(window))
        return "Fullscreen: On";
    if (button->action == TOGGLE_MUSIC) {
        if (!menu->music.track)
            return "Music unavailable";
        if (menu->music.muted)
            return "Music: Off";
    }
    return button->label;
}

static void draw(SDL_Renderer *renderer, SDL_Window *window,
                 const struct menu_assets *assets, const struct menu *menu)
{
    const SDL_Rect terrain = {40, 345, 1600, 900};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, assets->map, &terrain, NULL);
    blit(renderer, assets->title, 15, 5);
    blit(renderer, assets->logo, WIDTH - 155, 8);

    if (menu->screen == CREDITS) {
        blit(renderer, assets->credits_frame, 530, 110);
        SDL_SetTextureColorMod(assets->font, 53, 37, 25);
        draw_credits(renderer, assets, menu->credit_scroll);
        SDL_SetTextureColorMod(assets->font, 255, 255, 255);
    } else {
        blit(renderer, assets->top, 608, 230);
        blit(renderer, assets->bottom, 608, 562);
        if (menu->screen == OPTIONS)
            centered_text(renderer, assets, "Options", 7, WIDTH / 2, 325);
    }

    size_t count;
    const struct button *buttons = buttons_for(menu->screen, &count);
    for (size_t i = 0; i < count; ++i) {
        const struct button *button = &buttons[i];
        const bool enabled = action_enabled(menu, button->action);
        SDL_Texture *texture = enabled && button->action == menu->hover
            ? assets->button_hover : assets->button;
        const Uint8 brightness = !enabled ? 145 : button->action == menu->focus ? 255 : 235;
        const char *label = button_label(button, window, menu);

        SDL_SetTextureColorMod(texture, brightness, brightness, brightness);
        SDL_RenderCopy(renderer, texture, NULL, &button->area);
        SDL_SetTextureAlphaMod(assets->font, enabled ? 255 : 150);
        centered_text(renderer, assets, label, strlen(label), button->area.x + BUTTON_WIDTH / 2,
                      button->area.y + (BUTTON_HEIGHT - 22) / 2);
    }
    SDL_SetTextureAlphaMod(assets->font, 255);
    SDL_RenderPresent(renderer);
}

static enum action hit_test(enum screen screen, int x, int y)
{
    size_t count;
    const struct button *buttons = buttons_for(screen, &count);
    const SDL_Point point = {x, y};
    for (size_t i = 0; i < count; ++i) {
        if (SDL_PointInRect(&point, &buttons[i].area))
            return buttons[i].action;
    }
    return UNAVAILABLE;
}

static void activate(struct menu *menu, SDL_Window *window, enum action action)
{
    if (!action_enabled(menu, action))
        return;
    menu->hover = UNAVAILABLE;
    switch (action) {
    case OPEN_OPTIONS:
        menu->screen = OPTIONS;
        menu->focus = TOGGLE_FULLSCREEN;
        break;
    case OPEN_CREDITS:
        menu->screen = CREDITS;
        menu->credit_scroll = 0;
        menu->focus = BACK;
        break;
    case BACK:
        menu->focus = menu->screen == OPTIONS ? OPEN_OPTIONS : OPEN_CREDITS;
        menu->screen = MAIN_MENU;
        break;
    case QUIT:
        menu->running = false;
        break;
    case TOGGLE_FULLSCREEN:
        if (SDL_SetWindowFullscreen(window, fullscreen(window) ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP) < 0)
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Display mode", SDL_GetError(), window);
        break;
    case TOGGLE_MUSIC:
        menu_music_toggle(&menu->music);
        break;
    case UNAVAILABLE:
        break;
    }
}

static void move_focus(struct menu *menu, int direction)
{
    size_t count;
    const struct button *buttons = buttons_for(menu->screen, &count);
    int current = direction > 0 ? (int)count - 1 : 0;
    for (size_t i = 0; i < count; ++i) {
        if (action_enabled(menu, buttons[i].action) && buttons[i].action == menu->focus)
            current = (int)i;
    }
    do {
        current = (current + direction + (int)count) % (int)count;
    } while (!action_enabled(menu, buttons[current].action));
    menu->focus = buttons[current].action;
}

static void scroll_credits(struct menu *menu, int amount)
{
    menu->credit_scroll = SDL_clamp(menu->credit_scroll + amount, 0, menu->credit_scroll_max);
}

static void handle_event(struct menu *menu, SDL_Window *window, const SDL_Event *event)
{
    switch (event->type) {
    case SDL_QUIT:
        menu->running = false;
        break;
    case SDL_MOUSEMOTION:
        menu->hover = hit_test(menu->screen, event->motion.x, event->motion.y);
        break;
    case SDL_WINDOWEVENT:
        if (event->window.event == SDL_WINDOWEVENT_LEAVE)
            menu->hover = UNAVAILABLE;
        break;
    case SDL_MOUSEBUTTONUP:
        if (event->button.button == SDL_BUTTON_LEFT)
            activate(menu, window, hit_test(menu->screen, event->button.x, event->button.y));
        break;
    case SDL_MOUSEWHEEL:
        if (menu->screen == CREDITS) {
            int steps = SDL_clamp(event->wheel.y, -20, 20);
            if (event->wheel.direction == SDL_MOUSEWHEEL_FLIPPED)
                steps = -steps;
            scroll_credits(menu, -3 * CREDIT_LINE * steps);
        }
        break;
    case SDL_KEYDOWN:
        switch (event->key.keysym.sym) {
        case SDLK_ESCAPE:
            if (!event->key.repeat)
                activate(menu, window, menu->screen == MAIN_MENU ? QUIT : BACK);
            break;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
        case SDLK_SPACE:
            if (!event->key.repeat)
                activate(menu, window, menu->focus);
            break;
        case SDLK_UP:
        case SDLK_DOWN:
            if (menu->screen == CREDITS)
                scroll_credits(menu, event->key.keysym.sym == SDLK_UP ? -CREDIT_LINE : CREDIT_LINE);
            else
                move_focus(menu, event->key.keysym.sym == SDLK_UP ? -1 : 1);
            break;
        case SDLK_PAGEUP:
        case SDLK_PAGEDOWN:
            if (menu->screen == CREDITS)
                scroll_credits(menu, event->key.keysym.sym == SDLK_PAGEUP ? -480 : 480);
            break;
        }
        break;
    }
}

static int run_game(const char *root)
{
    struct menu_assets assets = {0};
    struct menu menu = {.screen = MAIN_MENU, .running = true};
    int status = EXIT_FAILURE;
    SDL_Window *window = SDL_CreateWindow("OpenCK2",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_HIDDEN);
    if (!window) {
        fprintf(stderr, "%s\n", SDL_GetError());
        return status;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer)
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);

    if (!renderer || SDL_RenderSetLogicalSize(renderer, WIDTH, HEIGHT) < 0 ||
        menu_assets_load(&assets, renderer, root) < 0) {
        fprintf(stderr, "Could not load the CK2 main menu: %s\n", SDL_GetError());
        goto cleanup;
    }

    menu_music_start(&menu.music, root);
    int lines = 1;
    for (const char *p = assets.credits; *p; ++p)
        lines += *p == '\n';
    menu.credit_scroll_max = SDL_max(0, lines * CREDIT_LINE - 535);
    SDL_ShowWindow(window);
    draw(renderer, window, &assets, &menu);

    SDL_Event event;
    while (menu.running && SDL_WaitEvent(&event)) {
        do {
            handle_event(&menu, window, &event);
        } while (menu.running && SDL_PollEvent(&event));
        if (menu.running)
            draw(renderer, window, &assets, &menu);
    }
    if (menu.running)
        fprintf(stderr, "Menu event loop stopped: %s\n", SDL_GetError());
    else
        status = EXIT_SUCCESS;

cleanup:
    menu_music_stop(&menu.music);
    menu_assets_free(&assets);
    if (renderer)
        SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    return status;
}

int main(int argc, char **argv)
{
    const char *root = SDL_getenv("CK2_DIR");
    char *default_root = NULL;
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        puts("Usage: ck2-game [--assets CK2_INSTALL_DIRECTORY]\n"
             "Loads the main menu from an existing Crusader Kings II installation.\n"
             "The asset directory can also be set with CK2_DIR.");
        return EXIT_SUCCESS;
    }
    if (argc == 3 && strcmp(argv[1], "--assets") == 0) {
        root = argv[2];
    } else if (argc != 1) {
        fputs("Usage: ck2-game [--assets CK2_INSTALL_DIRECTORY]\n", stderr);
        return EXIT_FAILURE;
    }

    if (!root || !*root) {
        const char *home = SDL_getenv("HOME");
        const char suffix[] = "/.steam/steam/steamapps/common/Crusader Kings II";
        if (home && *home) {
            const size_t size = strlen(home) + sizeof suffix;
            default_root = SDL_malloc(size);
            if (default_root)
                SDL_snprintf(default_root, size, "%s%s", home, suffix);
        }
        root = default_root;
    }
    if (!root) {
        fputs("Specify the local CK2 installation with --assets or CK2_DIR.\n", stderr);
        return EXIT_FAILURE;
    }

    int status = EXIT_FAILURE;
    if (SDL_Init(SDL_INIT_VIDEO) == 0)
        status = run_game(root);
    else
        fprintf(stderr, "Could not initialize SDL: %s\n", SDL_GetError());
    SDL_Quit();
    SDL_free(default_root);
    return status;
}
