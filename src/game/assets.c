#include "assets.h"
#include "../open_launcher/asset_paths.h"
#include <stdlib.h>

#include <SDL_image.h>
#include <stdio.h>
#include <string.h>

static Uint32 read_u32(const Uint8 *bytes)
{
    Uint32 value;
    memcpy(&value, bytes, sizeof value);
    return SDL_SwapLE32(value);
}

static void decode_bc3_block(SDL_Surface *surface, int x, int y, const Uint8 block[16])
{
    Uint8 colors[4][3];
    for (int i = 0; i < 2; ++i) {
        const unsigned rgb = block[8 + i * 2] | (unsigned)block[9 + i * 2] << 8;
        colors[i][0] = (rgb & 31) * 255 / 31;
        colors[i][1] = ((rgb >> 5) & 63) * 255 / 63;
        colors[i][2] = (rgb >> 11) * 255 / 31;
    }
    for (int i = 0; i < 3; ++i) {
        colors[2][i] = (2 * colors[0][i] + colors[1][i]) / 3;
        colors[3][i] = (colors[0][i] + 2 * colors[1][i]) / 3;
    }
    Uint8 alpha[8] = {block[0], block[1], 0, 0, 0, 0, 0, 255};
    const int steps = alpha[0] > alpha[1] ? 7 : 5;
    for (int i = 1; i < steps; ++i)
        alpha[i + 1] = ((steps - i) * alpha[0] + i * alpha[1]) / steps;
    Uint64 alphas = 0;
    for (int i = 0; i < 6; ++i)
        alphas |= (Uint64)block[2 + i] << (8 * i);
    const Uint32 indices = read_u32(block + 12);
    for (int row = 0; row < 4 && y + row < surface->h; ++row) {
        for (int col = 0; col < 4 && x + col < surface->w; ++col) {
            const int index = 4 * row + col;
            Uint8 *pixel = (Uint8 *)surface->pixels + (y + row) * surface->pitch + (x + col) * 4;
            memcpy(pixel, colors[(indices >> (2 * index)) & 3], 3);
            pixel[3] = alpha[(alphas >> (3 * index)) & 7];
        }
    }
}

static bool read_bc3(SDL_RWops *file, SDL_Surface *surface)
{
    for (int y = 0; y < surface->h; y += 4) {
        for (int x = 0; x < surface->w; x += 4) {
            Uint8 block[16];
            if (SDL_RWread(file, block, sizeof block, 1) != 1)
                return false;
            decode_bc3_block(surface, x, y, block);
        }
    }
    return true;
}

SDL_Surface *ck2_load_dds_stream(SDL_RWops *file)
{
    if (!file)
        return NULL;

    SDL_Surface *surface = NULL;
    Uint8 header[128];
    if (SDL_RWread(file, header, sizeof header, 1) != 1 ||
        memcmp(header, "DDS ", 4) != 0 || read_u32(header + 4) != 124 ||
        read_u32(header + 76) != 32) {
        SDL_SetError("Invalid DDS header");
        goto done;
    }

    const Uint32 flags = read_u32(header + 80);
    const bool bc3 = flags == 4 && memcmp(header + 84, "DXT5", 4) == 0;
    const bool rgb24 = flags == 0x40 && read_u32(header + 88) == 24;
    const bool rgba32 = flags == 0x41 && read_u32(header + 88) == 32 &&
        read_u32(header + 104) == 0xff000000;
    if (!bc3 && ((!rgb24 && !rgba32) || read_u32(header + 92) != 0x00ff0000 ||
        read_u32(header + 96) != 0x0000ff00 || read_u32(header + 100) != 0x000000ff)) {
        SDL_SetError("Expected BC3, RGB24, or ARGB8888 DDS");
        goto done;
    }
    const Uint32 height = read_u32(header + 12);
    const Uint32 width = read_u32(header + 16);
    const Uint32 row_bytes = width * (rgb24 ? 3 : 4);
    const Uint32 file_pitch = read_u32(header + 8) & 8 ? read_u32(header + 20) : row_bytes;
    if (!width || !height || width > 8192 || height > 8192 ||
        (!bc3 && (file_pitch < row_bytes || file_pitch > row_bytes + 3))) {
        SDL_SetError("Unsupported DDS dimensions or row pitch");
        goto done;
    }

    surface = SDL_CreateRGBSurfaceWithFormat(0, (int)width, (int)height, 32,
                                             SDL_PIXELFORMAT_BGRA32);
    bool read = surface != NULL;
    if (surface && bc3)
        read = read_bc3(file, surface);
    else if (surface && rgb24) {
        Uint8 row[8192 * 3 + 3];
        for (Uint32 y = 0; y < height && read; ++y) {
            read = SDL_RWread(file, row, file_pitch, 1) == 1;
            if (!read)
                break;
            Uint8 *out = (Uint8 *)surface->pixels + (size_t)y * surface->pitch;
            for (Uint32 x = 0; x < width; ++x) {
                memcpy(out + x * 4, row + x * 3, 3);
                out[x * 4 + 3] = 255;
            }
        }
    } else if (surface) {
        read = SDL_RWread(file, surface->pixels, file_pitch, height) == height;
    }
    if (surface && !read) {
        SDL_FreeSurface(surface);
        surface = NULL;
        SDL_SetError("Truncated DDS pixel data");
    }

done:
    SDL_RWclose(file);
    return surface;
}

static SDL_Surface *load_dds(const char *path)
{
    return ck2_load_dds_stream(SDL_RWFromFile(path, "rb"));
}

SDL_Surface *ck2_load_dds_surface(const char *root, const char *relative)
{
    char *path = ck2_asset_path(root, relative);
    if (!path)
        return NULL;
    SDL_Surface *surface = load_dds(path);
    free(path);
    return surface;
}

static SDL_Texture *load_texture(SDL_Renderer *renderer, const char *root,
                                 const char *relative, bool gray)
{
    char *path = ck2_asset_path(root, relative);
    if (!path)
        return NULL;

    const size_t length = strlen(path);
    SDL_Surface *surface = length >= 4 && strcmp(path + length - 4, ".dds") == 0
        ? load_dds(path) : IMG_Load(path);
    if (surface && gray && surface->format->format != SDL_PIXELFORMAT_BGRA32) {
        SDL_Surface *converted = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_BGRA32, 0);
        SDL_FreeSurface(surface);
        surface = converted;
    }
    if (surface && gray) {
        /* Keep the original silhouette and alpha. */
        for (int y = 0; y < surface->h; ++y) {
            Uint8 *pixel = (Uint8 *)surface->pixels + y * surface->pitch;
            for (int x = 0; x < surface->w; ++x, pixel += 4) {
                const int luminance = (29 * pixel[0] + 150 * pixel[1] + 77 * pixel[2]) / 256;
                const Uint8 highlight = (Uint8)SDL_min(255, luminance + 48);
                pixel[0] = pixel[1] = pixel[2] = highlight;
            }
        }
    }
    SDL_Texture *texture = surface ? SDL_CreateTextureFromSurface(renderer, surface) : NULL;
    if (!texture)
        fprintf(stderr, "%s: %s\n", path, SDL_GetError());
    SDL_FreeSurface(surface);
    free(path);
    return texture;
}

SDL_Texture *ck2_load_texture(SDL_Renderer *renderer, const char *root, const char *relative)
{
    return load_texture(renderer, root, relative, false);
}

SDL_Texture *ck2_load_hover(SDL_Renderer *renderer, const char *root, const char *relative)
{
    return load_texture(renderer, root, relative, true);
}

static char *load_text(const char *root, const char *relative)
{
    char *path = ck2_asset_path(root, relative);
    if (!path)
        return NULL;
    char *text = SDL_LoadFile(path, NULL);
    if (!text)
        fprintf(stderr, "%s: %s\n", path, SDL_GetError());
    free(path);
    return text;
}

int ck2_load_font_metrics(struct glyph glyphs[256], const char *root, const char *relative)
{
    char *font = load_text(root, relative);
    if (!font)
        return -1;

    for (char *line = strtok(font, "\n"); line; line = strtok(NULL, "\n")) {
        unsigned id;
        struct glyph glyph;
        if (sscanf(line, "char id=%u x=%d y=%d width=%d height=%d xoffset=%d yoffset=%d xadvance=%d",
                   &id, &glyph.area.x, &glyph.area.y, &glyph.area.w, &glyph.area.h,
                   &glyph.x_offset, &glyph.y_offset, &glyph.advance) == 8 && id < 256)
            glyphs[id] = glyph;
    }
    SDL_free(font);
    if (glyphs['A'].advance <= 0)
        return SDL_SetError("Invalid font metrics for %s", relative);
    return 0;
}

int ck2_load_glyphs(struct glyph glyphs[256], const char *root)
{
    return ck2_load_font_metrics(glyphs, root, "gfx/fonts/vic_22.fnt");
}

int menu_assets_load(struct menu_assets *assets, SDL_Renderer *renderer, const char *root)
{
    struct texture_asset {
        SDL_Texture **texture;
        const char *relative;
    } textures[] = {
        {&assets->title, "gfx/interface/frontend_title.dds"},
        {&assets->logo, "gfx/interface/pdx_dev_logo.dds"},
        {&assets->top, "gfx/interface/frontend_center_panel_top.dds"},
        {&assets->bottom, "gfx/interface/frontend_center_panel_bottom1.dds"},
        {&assets->bottom_content, "gfx/interface/frontend_center_panel_bottom2.dds"},
        {&assets->compact_button, "gfx/interface/standard_button_164.dds"},
        {&assets->version_plate, "gfx/interface/frontend_version.dds"},
        {&assets->button, "gfx/interface/big_button_232.dds"},
        {&assets->credits_frame, "gfx/interface/creditscreen_bg.dds"},
        {&assets->dialog_frame, "gfx/interface/menu.dds"},
        {&assets->dialog_heading, "gfx/interface/menu_settings_bg.dds"},
        {&assets->hud_ruler, "gfx/interface/topbar_bg.dds"},
        {&assets->hud_resources_left, "gfx/interface/topbar_right_bg_left.dds"},
        {&assets->hud_resources_right, "gfx/interface/topbar_right_bg_right.dds"},
        {&assets->hud_date, "gfx/interface/topbar_date_pause_small.dds"},
        {&assets->hud_minimap, "gfx/interface/minimap_bg.dds"},
        {&assets->hud_map, "gfx/interface/minimap_area_bg.dds"},
        {&assets->hud_province, "gfx/interface/province_bg.dds"},
        {&assets->hud_outliner, "gfx/interface/outliner_top.dds"},
        {&assets->hud_arms, "gfx/flags/c_york.tga"},
        {&assets->religion_icons, "gfx/interface/religion_icon_strip.dds"},
        {&assets->lobby_tooltip, "gfx/interface/tooltip_bg.dds"},
        {&assets->lobby_row, "gfx/interface/frontend_list_button.dds"},
        {&assets->holding_art[0], "gfx/interface/province_new_settlement.tga"},
        {&assets->holding_art[1], "gfx/interface/placeholder_castle.tga"},
        {&assets->holding_art[2], "gfx/interface/placeholder_city.tga"},
        {&assets->holding_art[3], "gfx/interface/placeholder_temple.tga"},
        {&assets->holding_art[4], "gfx/interface/placeholder_tribal.tga"},
        {&assets->holding_art[5], "gfx/interface/placeholder_nomad.dds"},
        {&assets->holding_frame, "gfx/interface/province_settlement_frame.dds"},
        {&assets->holding_capital, "gfx/interface/province_capital_settlement_frame.dds"},
        {&assets->hud_tabs[0], "gfx/interface/topbar_folder_court.dds"},
        {&assets->hud_tabs[1], "gfx/interface/topbar_folder_laws.dds"},
        {&assets->hud_tabs[2], "gfx/interface/topbar_folder_tech.dds"},
        {&assets->hud_tabs[3], "gfx/interface/topbar_folder_military.dds"},
        {&assets->hud_tabs[4], "gfx/interface/topbar_folder_plots.dds"},
        {&assets->hud_tabs[5], "gfx/interface/topbar_folder_factions.dds"},
        {&assets->hud_tabs[6], "gfx/interface/topbar_folder_religion.dds"},
        {&assets->hud_tabs[7], "gfx/interface/topbar_folder_societies.dds"},
        {&assets->font, "gfx/fonts/vic_22.tga"},
    };

    for (size_t i = 0; i < SDL_arraysize(textures); ++i) {
        *textures[i].texture = ck2_load_texture(renderer, root, textures[i].relative);
        if (!*textures[i].texture)
            return -1;
    }

    assets->button_hover = ck2_load_hover(renderer, root, "gfx/interface/big_button_232.dds");
    assets->compact_hover = ck2_load_hover(renderer, root, "gfx/interface/standard_button_164.dds");
    if (!assets->button_hover || !assets->compact_hover)
        return -1;
    for (int i = 0; i < CK2_DLC_ICON_COUNT; ++i)
        assets->dlc_icons[i] = ck2_load_texture(renderer, root, ck2_catalog_icon_path(i));

    assets->credits = load_text(root, "interface/credits.txt");
    return assets->credits ? ck2_load_glyphs(assets->glyphs, root) : -1;
}

void menu_assets_free(struct menu_assets *assets)
{
    SDL_Texture *textures[] = {
        assets->title, assets->logo, assets->top, assets->bottom,
        assets->button, assets->credits_frame, assets->font,
        assets->dialog_frame, assets->dialog_heading, assets->hud_ruler,
        assets->hud_resources_left, assets->hud_resources_right, assets->hud_date,
        assets->hud_minimap, assets->hud_map, assets->hud_province,
        assets->hud_outliner, assets->hud_arms,
        assets->lobby_tooltip, assets->religion_icons, assets->lobby_row,
        assets->holding_art[0], assets->holding_art[1], assets->holding_art[2],
        assets->holding_art[3], assets->holding_art[4], assets->holding_art[5],
        assets->holding_frame, assets->holding_capital
    };
    for (size_t i = 0; i < SDL_arraysize(textures); ++i) {
        if (textures[i])
            SDL_DestroyTexture(textures[i]);
    }
    for (int i = 0; i < CK2_DLC_ICON_COUNT; ++i) {
        if (assets->dlc_icons[i])
            SDL_DestroyTexture(assets->dlc_icons[i]);
    }
    for (size_t i = 0; i < SDL_arraysize(assets->hud_tabs); ++i) {
        if (assets->hud_tabs[i])
            SDL_DestroyTexture(assets->hud_tabs[i]);
    }
    SDL_free(assets->credits);
}
