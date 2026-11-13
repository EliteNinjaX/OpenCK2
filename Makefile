CFLAGS ?= -O2
BUILD_DIR ?= bin

.PHONY: all
all: game

GAME := $(BUILD_DIR)/OpenCK2
CONTENT_CATALOG := src/open_launcher/asset_paths.c src/open_launcher/zip_assets.c src/open_launcher/install.c src/open_launcher/catalog.c
CONTENT_HEADERS := src/open_launcher/zip_assets.h src/open_launcher/asset_paths.h src/open_launcher/install.h src/open_launcher/catalog.h
GAME_SOURCES := src/game/checkpoint_core.c src/game/election_core.c src/game/law_core.c src/game/wonder_navigation_core.c src/game/bloodline_browser_core.c src/game/bloodline_view.c src/game/warfare_core.c src/game/warfare_map.c src/game/event_core.c src/game/conversion_core.c src/game/political_map_core.c src/game/family_gameplay.c src/game/family_names_core.c src/game/music_playlist_core.c src/game/map_interaction.c src/game/map_modes.c src/game/map_themes.c src/game/map_theme_core.c src/game/map_palette_core.c src/game/levy_core.c src/game/campaign_hud.c src/game/ck2_lexer_core.c src/game/technology_core.c src/game/technology_view.c src/game/debug_console.c src/game/government_core.c src/game/government_mechanics_core.c src/game/governments.c src/game/settlement_styles.c src/game/map_positions.c src/game/map_settlements.c src/game/economy_core.c src/game/title_prestige.c src/game/economy.c src/game/main.c src/game/province_layout_core.c src/game/sprite_frame_core.c src/game/religion_style_cache.c src/game/religion_gfx_core.c src/game/tree_history.c src/game/tree_layout_core.c src/game/tree_view.c src/game/character_view.c src/game/dynasty_arms.c src/game/dynasty_arms_core.c src/game/assets.c src/game/music.c src/game/settings.c src/game/era.c src/game/portraits.c src/game/ck2_script.c src/game/localization_core.c src/game/bloodline.c src/game/rules.c src/game/realms.c src/game/holdings.c src/game/construction.c src/game/rulers.c src/game/campaign.c src/game/map_view.c src/game/models.c src/game/setup_toolbar.c src/game/frontend_gui.c $(CONTENT_CATALOG)

.PHONY: game
game: open-launcher

$(GAME): $(GAME_SOURCES) src/game/checkpoint_core.h src/game/election_core.h src/game/law_core.h src/game/wonder_navigation_core.h src/game/bloodline_browser_core.h src/game/bloodline_view.h src/game/warfare_core.h src/game/warfare_map.h src/game/event_core.h src/game/conversion_core.h src/game/political_map_core.h src/game/family_gameplay.h src/game/family_names_core.h src/game/music_playlist_core.h src/game/map_interaction.h src/game/map_modes.h src/game/map_themes.h src/game/map_theme_core.h src/game/map_palette_core.h src/game/levy_core.h src/game/campaign_hud.h src/game/ck2_lexer_core.h src/game/technology_core.h src/game/technology_view.h src/game/debug_console.h src/game/government_core.h src/game/government_mechanics_core.h src/game/governments.h src/game/settlement_styles.h src/game/map_positions.h src/game/map_settlements.h src/game/economy_core.h src/game/title_prestige.h src/game/economy.h src/game/province_layout_core.h src/game/sprite_frame_core.h src/game/religion_style_cache.h src/game/religion_gfx_core.h src/game/character_view.h src/game/tree_history.h src/game/tree_layout_core.h src/game/tree_view.h src/game/localization_core.h src/game/dynasty_arms.h src/game/dynasty_arms_core.h src/game/assets.h src/game/music.h src/game/settings.h src/game/era.h src/game/portraits.h src/game/ck2_script.h src/game/bloodline.h src/game/rules.h src/game/realms.h src/game/holdings.h src/game/construction.h src/game/rulers.h src/game/campaign.h src/game/map_view.h src/game/models.h src/game/setup_toolbar.h src/game/frontend_gui.h src/version.h $(CONTENT_HEADERS) Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c17 -Wall -Wextra -Wpedantic \
		$$(pkg-config --cflags sdl2 SDL2_image SDL2_mixer zlib) $(LDFLAGS) -o "$@" $(GAME_SOURCES) \
		$$(pkg-config --libs sdl2 SDL2_image SDL2_mixer zlib) -lm $(LDLIBS)

OPEN_LAUNCHER := $(BUILD_DIR)/Launcher
OPEN_LAUNCHER_SOURCES := src/open_launcher/main.c $(CONTENT_CATALOG) src/game/assets.c

.PHONY: open-launcher
open-launcher: $(OPEN_LAUNCHER) $(GAME)

ifneq ($(wildcard assets/OpenCK2-openck2-background.png),)
open-launcher: $(BUILD_DIR)/OpenCK2-openck2-background.png
$(BUILD_DIR)/OpenCK2-openck2-background.png: assets/OpenCK2-openck2-background.png
	mkdir -p "$(@D)"
	cp "$<" "$@"
endif

$(OPEN_LAUNCHER): $(OPEN_LAUNCHER_SOURCES) $(CONTENT_HEADERS) src/game/assets.h src/version.h Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c17 -Wall -Wextra -Wpedantic \
		$$(pkg-config --cflags sdl2 SDL2_image zlib) $(LDFLAGS) -o "$@" $(OPEN_LAUNCHER_SOURCES) \
		$$(pkg-config --libs sdl2 SDL2_image zlib) $(LDLIBS)

CAMPAIGN_TEST := $(BUILD_DIR)/campaign-test
CAMPAIGN_TEST_SOURCES := tests/campaign.c src/game/campaign.c src/open_launcher/install.c src/open_launcher/asset_paths.c src/open_launcher/zip_assets.c

.PHONY: check-campaign
check-campaign: $(CAMPAIGN_TEST)
	"$(CAMPAIGN_TEST)"

$(CAMPAIGN_TEST): $(CAMPAIGN_TEST_SOURCES) src/game/campaign.h src/game/map_view.h src/open_launcher/install.h src/open_launcher/asset_paths.h src/open_launcher/zip_assets.h Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG -std=c17 -Wall -Wextra -Wpedantic -Isrc/game \
		$$(pkg-config --cflags sdl2) $(LDFLAGS) -o "$@" $(CAMPAIGN_TEST_SOURCES) \
		$$(pkg-config --libs sdl2 zlib) $(LDLIBS)

TITLE_HISTORY := $(BUILD_DIR)/title-history

.PHONY: title-history
title-history: $(TITLE_HISTORY)

$(TITLE_HISTORY): tools/title-history.c Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c17 -Wall -Wextra -Wpedantic $(LDFLAGS) -o "$@" "$<" $(LDLIBS)

TOOLBAR_TEST := $(BUILD_DIR)/setup-toolbar-test
TOOLBAR_TEST_SOURCES := tests/setup-toolbar.c src/game/setup_toolbar.c src/game/frontend_gui.c src/game/assets.c src/game/ck2_script.c src/game/localization_core.c $(CONTENT_CATALOG)

.PHONY: check-setup-toolbar
check-setup-toolbar: $(TOOLBAR_TEST)
	"$(TOOLBAR_TEST)"

$(TOOLBAR_TEST): $(TOOLBAR_TEST_SOURCES) src/game/setup_toolbar.h src/game/frontend_gui.h src/game/assets.h src/game/ck2_script.h $(CONTENT_HEADERS) Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG -std=c17 -Wall -Wextra -Wpedantic -Isrc/game -Isrc/open_launcher \
		$$(pkg-config --cflags sdl2 SDL2_image zlib) $(LDFLAGS) -o "$@" $(TOOLBAR_TEST_SOURCES) \
		$$(pkg-config --libs sdl2 SDL2_image zlib) $(LDLIBS)

HOLDING_SOURCES := src/game/holdings.c src/game/campaign.c src/game/ck2_script.c src/game/localization_core.c $(CONTENT_CATALOG)
HOLDING_HEADERS := src/game/holdings.h src/game/campaign.h src/game/ck2_script.h src/game/map_view.h $(CONTENT_HEADERS)
HOLDING_AUDIT := $(BUILD_DIR)/holding-audit
HOLDING_TEST := $(BUILD_DIR)/holdings-test

.PHONY: holding-audit check-holdings
holding-audit: $(HOLDING_AUDIT)

check-holdings: $(HOLDING_TEST)
	"$(HOLDING_TEST)"

$(HOLDING_AUDIT): tools/holding-audit.c $(HOLDING_SOURCES) $(HOLDING_HEADERS) Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c17 -Wall -Wextra -Wpedantic -Isrc/game \
		$$(pkg-config --cflags sdl2 zlib) $(LDFLAGS) -o "$@" "$<" $(HOLDING_SOURCES) \
		$$(pkg-config --libs sdl2 zlib) $(LDLIBS)

$(HOLDING_TEST): tests/holdings.c $(HOLDING_SOURCES) $(HOLDING_HEADERS) Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG -std=c17 -Wall -Wextra -Wpedantic -Isrc/game -Isrc/open_launcher \
		$$(pkg-config --cflags sdl2 zlib) $(LDFLAGS) -o "$@" "$<" $(HOLDING_SOURCES) \
		$$(pkg-config --libs sdl2 zlib) $(LDLIBS)

PORTRAIT_TEST := $(BUILD_DIR)/portraits-test
PORTRAIT_TEST_SOURCES := tests/portraits.c src/game/assets.c src/game/ck2_script.c src/game/localization_core.c src/game/rulers.c src/game/portraits.c $(CONTENT_CATALOG)

.PHONY: check-portraits
check-portraits: $(PORTRAIT_TEST)
	"$(PORTRAIT_TEST)" "$${CK2_DIR:-$$HOME/.steam/steam/steamapps/common/Crusader Kings II}"

$(PORTRAIT_TEST): $(PORTRAIT_TEST_SOURCES) src/game/portraits.h src/game/era.h src/game/rulers.h src/game/assets.h src/game/ck2_script.h $(CONTENT_HEADERS) Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG -std=c17 -Wall -Wextra -Wpedantic -Isrc/game -Isrc/open_launcher \
		$$(pkg-config --cflags sdl2 SDL2_image zlib) $(LDFLAGS) -o "$@" $(PORTRAIT_TEST_SOURCES) \
		$$(pkg-config --libs sdl2 SDL2_image zlib) $(LDLIBS)

CONSTRUCTION_TEST := $(BUILD_DIR)/construction-test
.PHONY: check-construction
check-construction: $(CONSTRUCTION_TEST)
	"$(CONSTRUCTION_TEST)"

$(CONSTRUCTION_TEST): tests/construction.c src/game/construction.c src/game/construction.h src/game/ck2_script.c src/game/localization_core.c $(CONTENT_CATALOG) $(CONTENT_HEADERS)
	mkdir -p "$(dir $@)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG -std=c17 -Wall -Wextra -Wpedantic -Isrc/game \
		$$(pkg-config --cflags sdl2 zlib) $(LDFLAGS) -o "$@" tests/construction.c src/game/construction.c src/game/levy_core.c src/game/ck2_script.c src/game/localization_core.c $(CONTENT_CATALOG) \
		$$(pkg-config --libs sdl2 zlib) $(LDLIBS)

# Independent, SDL-free C17 tests: available even without proprietary assets or SDL headers.
MAP_CORE_TEST := $(BUILD_DIR)/map-core-test
.PHONY: check-map-core
check-map-core: $(MAP_CORE_TEST)
	"$(MAP_CORE_TEST)"

$(MAP_CORE_TEST): tests/map-core.c src/game/map_interaction.c src/game/map_interaction.h src/game/map_modes.c src/game/map_modes.h Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c17 -Wall -Wextra -Wpedantic -o "$@" tests/map-core.c src/game/map_interaction.c src/game/map_modes.c $(LDLIBS)

# Original-data parsers are pure C17: these checks require no SDL, CK2 runtime,
# Windows DLLs, or installation-specific paths.
.PHONY: check-governments-core check-settlement-styles check-map-positions check-ck2-map-data
check-governments-core: $(BUILD_DIR)/governments-core-test
	"$(BUILD_DIR)/governments-core-test" $(CK2_GOVERNMENT_TEST_FILES)
$(BUILD_DIR)/governments-core-test: tests/government-core.c src/game/government_core.c src/game/ck2_lexer_core.c src/game/government_core.h src/game/ck2_lexer_core.h Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c17 -Wall -Wextra -Wpedantic -o "$@" tests/government-core.c src/game/government_core.c src/game/ck2_lexer_core.c $(LDLIBS)
check-settlement-styles: $(BUILD_DIR)/settlement-styles-test
	"$(BUILD_DIR)/settlement-styles-test" $(CK2_CULTURES_TEST_FILES)
$(BUILD_DIR)/settlement-styles-test: tests/settlement-styles.c src/game/settlement_styles.c src/game/ck2_lexer_core.c src/game/settlement_styles.h src/game/ck2_lexer_core.h Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c17 -Wall -Wextra -Wpedantic -o "$@" tests/settlement-styles.c src/game/settlement_styles.c src/game/ck2_lexer_core.c $(LDLIBS)
check-map-positions: $(BUILD_DIR)/map-positions-test
	"$(BUILD_DIR)/map-positions-test"
$(BUILD_DIR)/map-positions-test: tests/map-positions.c src/game/map_positions.c src/game/ck2_lexer_core.c src/game/map_positions.h src/game/ck2_lexer_core.h Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c17 -Wall -Wextra -Wpedantic -o "$@" tests/map-positions.c src/game/map_positions.c src/game/ck2_lexer_core.c $(LDLIBS)
check-ck2-map-data: check-governments-core check-settlement-styles check-map-positions

# Portable economy parsing test, plus SDL-free holding history/tax fixture.
.PHONY: check-economy-core check-finance-holdings check-economy
check-economy-core: $(BUILD_DIR)/economy-core-test
	"$(BUILD_DIR)/economy-core-test"
$(BUILD_DIR)/economy-core-test: tests/economy-core.c src/game/economy_core.c src/game/economy_core.h Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c17 -Wall -Wextra -Wpedantic -Werror -Isrc/game -o "$@" tests/economy-core.c src/game/economy_core.c -lm

# The small stub exposes types only. Unused SDL/game-data entry points are
# removed at link time; tax/history assertions exercise real production code.
check-finance-holdings: $(BUILD_DIR)/finance-holdings-test
	"$(BUILD_DIR)/finance-holdings-test"
$(BUILD_DIR)/finance-holdings-test: tests/finance-holdings.c src/game/holdings.c src/game/construction.c src/game/levy_core.c src/game/ck2_script.c src/game/localization_core.c Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c17 -Wall -Wextra -Wpedantic -Werror -ffunction-sections -fdata-sections \
		-Itests/stubs -Isrc/game -o "$@" tests/finance-holdings.c src/game/holdings.c \
		src/game/construction.c src/game/levy_core.c src/game/ck2_script.c src/game/localization_core.c -Wl,--gc-sections -lm
check-economy: check-economy-core check-finance-holdings

# SDL-independent technology definitions and developer console tests.
.PHONY: check-technology-console
check-technology-console: $(BUILD_DIR)/technology-console-test
	"$(BUILD_DIR)/technology-console-test" $(CK2_TECHNOLOGY_TEST_FILE)

$(BUILD_DIR)/technology-console-test: tests/technology-console.c src/game/technology_core.c src/game/technology_core.h src/game/debug_console.c src/game/debug_console.h src/game/ck2_lexer_core.c src/game/ck2_lexer_core.h Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c17 -Wall -Wextra -Wpedantic -Werror -Isrc/game $(LDFLAGS) -o "$@" tests/technology-console.c src/game/technology_core.c src/game/debug_console.c src/game/ck2_lexer_core.c $(LDLIBS)

# Authored CK2 dynasty coat-of-arms parser: SDL-free validation and fixtures.
.PHONY: check-dynasty-arms
check-dynasty-arms: $(BUILD_DIR)/dynasty-arms-test
	"$(BUILD_DIR)/dynasty-arms-test" $(if $(CK2_DIR),"$(CK2_DIR)",)
$(BUILD_DIR)/dynasty-arms-test: tests/dynasty-arms.c src/game/dynasty_arms_core.c src/game/dynasty_arms_core.h src/game/ck2_lexer_core.c src/game/ck2_lexer_core.h Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c17 -Wall -Wextra -Wpedantic -Werror -o "$@" tests/dynasty-arms.c src/game/dynasty_arms_core.c src/game/ck2_lexer_core.c

# SDL-free CK2 multilingual CSV parser regression tests.
.PHONY: check-localization-core
check-localization-core: $(BUILD_DIR)/localization-core-test
	"$(BUILD_DIR)/localization-core-test"
$(BUILD_DIR)/localization-core-test: tests/localization-core.c src/game/localization_core.c src/game/localization_core.h
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c17 -Wall -Wextra -Wpedantic -o "$@" tests/localization-core.c src/game/localization_core.c $(LDLIBS)

.PHONY: check-tree-history
check-tree-history: $(BUILD_DIR)/tree-history-test
	"$(BUILD_DIR)/tree-history-test"
$(BUILD_DIR)/tree-history-test: tests/tree-history.c src/game/tree_history.c src/game/tree_history.h src/game/ck2_lexer_core.c src/game/ck2_lexer_core.h
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c17 -Wall -Wextra -Wpedantic -o "$@" tests/tree-history.c src/game/tree_history.c src/game/ck2_lexer_core.c $(LDLIBS)

.PHONY: check-tree-layout
check-tree-layout: $(BUILD_DIR)/tree-layout-test
	"$(BUILD_DIR)/tree-layout-test"
$(BUILD_DIR)/tree-layout-test: tests/tree-layout.c src/game/tree_layout_core.c src/game/tree_layout_core.h
	@mkdir -p "$(BUILD_DIR)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c17 -Wall -Wextra -Wpedantic -o "$@" tests/tree-layout.c src/game/tree_layout_core.c $(LDLIBS)

# SDL-free regression for new holding completion -> economy -> demesne updates.
# Unused SDL asset code is removed at link time; all gameplay functions below
# are the same definitions linked into OpenCK2, not reimplemented test doubles.
.PHONY: check-economy-construction
check-economy-construction: $(BUILD_DIR)/economy-construction-test
	"$(BUILD_DIR)/economy-construction-test"

$(BUILD_DIR)/economy-construction-test: tests/economy-construction.c src/game/economy.c src/game/economy.h src/game/construction.c src/game/construction.h Makefile
	mkdir -p "$(dir $@)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG -std=c17 -Wall -Wextra -Wpedantic -Werror \
		-ffunction-sections -fdata-sections -Itests/stubs -Isrc/game $(LDFLAGS) \
		-Wl,--gc-sections -o "$@" tests/economy-construction.c \
		src/game/economy.c src/game/title_prestige.c src/game/construction.c src/game/levy_core.c src/game/ck2_script.c -lm $(LDLIBS)

.PHONY: check-title-prestige
check-title-prestige:
	mkdir -p "$(BUILD_DIR)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG -std=c17 -Wall -Wextra -Wpedantic -Werror -o "$(BUILD_DIR)/title-prestige-test" tests/title-prestige.c src/game/title_prestige.c
	"$(BUILD_DIR)/title-prestige-test"

# Pure C regression: all settlement entries must fit the visible CK2 panel.
PROVINCE_LAYOUT_TEST := $(BUILD_DIR)/province-layout-test
.PHONY: check-province-layout
check-province-layout: $(PROVINCE_LAYOUT_TEST)
	"$(PROVINCE_LAYOUT_TEST)"
$(PROVINCE_LAYOUT_TEST): tests/province-layout.c src/game/province_layout_core.c src/game/province_layout_core.h Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG -std=c17 -Wall -Wextra -Wpedantic -Werror -o "$@" tests/province-layout.c src/game/province_layout_core.c

RELIGION_GFX_TEST := $(BUILD_DIR)/religion-gfx-test
.PHONY: check-religion-gfx
check-religion-gfx: $(RELIGION_GFX_TEST)
	"$(RELIGION_GFX_TEST)"
$(RELIGION_GFX_TEST): tests/religion-gfx.c src/game/religion_gfx_core.c src/game/ck2_lexer_core.c src/game/religion_gfx_core.h Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG -std=c17 -Wall -Wextra -Wpedantic -Werror -o "$@" tests/religion-gfx.c src/game/religion_gfx_core.c src/game/ck2_lexer_core.c

# Horizontal CK2 sprite-sheet cell geometry (SDL-free).
.PHONY: check-sprite-frame
check-sprite-frame: $(BUILD_DIR)/sprite-frame-test
	"$(BUILD_DIR)/sprite-frame-test"

$(BUILD_DIR)/sprite-frame-test: tests/sprite-frame-core.c src/game/sprite_frame_core.c src/game/sprite_frame_core.h Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG -std=c17 -Wall -Wextra -Wpedantic -Werror -o "$@" tests/sprite-frame-core.c src/game/sprite_frame_core.c $(LDLIBS)

.PHONY: check-religion-style-cache
check-religion-style-cache: $(BUILD_DIR)/religion-style-cache-test
	"$(BUILD_DIR)/religion-style-cache-test"

$(BUILD_DIR)/religion-style-cache-test: tests/religion-style-cache.c src/game/religion_style_cache.c src/game/religion_style_cache.h Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG -std=c17 -Wall -Wextra -Wpedantic -Werror -o "$@" tests/religion-style-cache.c src/game/religion_style_cache.c $(LDLIBS)

# New SDL-independent core checks; no game installation necessary.
.PHONY: check-levy-core check-map-themes-core
check-levy-core: $(BUILD_DIR)/levy-core-test
	"$(BUILD_DIR)/levy-core-test"
$(BUILD_DIR)/levy-core-test: tests/levy-core.c src/game/levy_core.c src/game/levy_core.h
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG -std=c17 -Wall -Wextra -Wpedantic -Werror -o "$@" tests/levy-core.c src/game/levy_core.c -lm

check-map-themes-core: $(BUILD_DIR)/map-themes-core-test
	"$(BUILD_DIR)/map-themes-core-test"
$(BUILD_DIR)/map-themes-core-test: tests/map-themes-core.c src/game/map_theme_core.c src/game/map_theme_core.h
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG -std=c17 -Wall -Wextra -Wpedantic -Werror -o "$@" tests/map-themes-core.c src/game/map_theme_core.c -lm

.PHONY: check-map-palette
check-map-palette: $(BUILD_DIR)/map-palette-core-test
	"$(BUILD_DIR)/map-palette-core-test" $(CK2_PALETTE_FILE)
$(BUILD_DIR)/map-palette-core-test: tests/map-palette-core.c src/game/map_palette_core.c src/game/map_palette_core.h src/game/ck2_lexer_core.c
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG -std=c17 -Wall -Wextra -Wpedantic -Werror -o "$@" tests/map-palette-core.c src/game/map_palette_core.c src/game/ck2_lexer_core.c -lm

.PHONY: check-levy-buildings
check-levy-buildings: $(BUILD_DIR)/levy-buildings-test
	"$(BUILD_DIR)/levy-buildings-test" "$(CK2_CASTLE_BUILDING_FILE)"
$(BUILD_DIR)/levy-buildings-test: tests/levy-buildings.c src/game/construction.c src/game/levy_core.c src/game/ck2_script.c
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG -std=c17 -Wall -Wextra -Wpedantic -Werror \
		-ffunction-sections -fdata-sections -Itests/stubs -Isrc/game $(LDFLAGS) \
		-Wl,--gc-sections -o "$@" tests/levy-buildings.c src/game/levy_core.c src/game/ck2_script.c -lm $(LDLIBS)

# SDL-independent checks for identity-based political borders and runtime family events.
.PHONY: check-political-core check-family-gameplay
check-political-core: $(BUILD_DIR)/political-map-test
	"$(BUILD_DIR)/political-map-test"
$(BUILD_DIR)/political-map-test: tests/political-map-core.c src/game/political_map_core.c src/game/political_map_core.h Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG -std=c17 -Wall -Wextra -Wpedantic -Werror -Isrc/game -o "$@" tests/political-map-core.c src/game/political_map_core.c

check-family-gameplay: $(BUILD_DIR)/family-gameplay-test
	"$(BUILD_DIR)/family-gameplay-test"
$(BUILD_DIR)/family-gameplay-test: tests/family-gameplay.c src/game/family_gameplay.c src/game/family_gameplay.h Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG -std=c17 -Wall -Wextra -Wpedantic -Werror -Isrc/game -Itests/stubs -o "$@" tests/family-gameplay.c src/game/family_gameplay.c src/game/economy_core.c -lm

.PHONY: check-family-names
check-family-names: $(BUILD_DIR)/family-names-test
	"$(BUILD_DIR)/family-names-test" $(CK2_CULTURES_TEST_FILE)
$(BUILD_DIR)/family-names-test: tests/family-names.c src/game/family_names_core.c src/game/ck2_lexer_core.c src/game/family_names_core.h Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG -std=c17 -Wall -Wextra -Wpedantic -Werror -Isrc/game -o "$@" tests/family-names.c src/game/family_names_core.c src/game/ck2_lexer_core.c

# CK2 authored province and character conversion data; SDL independent.
.PHONY: check-conversion-core
check-conversion-core: $(BUILD_DIR)/conversion-core-test
	"$(BUILD_DIR)/conversion-core-test" $(CK2_CONVERSION_FILES)
$(BUILD_DIR)/conversion-core-test: tests/conversion-core.c src/game/conversion_core.c src/game/conversion_core.h src/game/ck2_lexer_core.c src/game/ck2_lexer_core.h Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c17 -Wall -Wextra -Wpedantic -Werror -o "$@" tests/conversion-core.c src/game/conversion_core.c src/game/ck2_lexer_core.c

# Scripted CK2 events: parsing and direct-effect safety checks (SDL independent).
.PHONY: check-events
check-events: $(BUILD_DIR)/event-core-test
	"$(BUILD_DIR)/event-core-test"
$(BUILD_DIR)/event-core-test: tests/event-core.c src/game/event_core.c src/game/event_core.h src/game/ck2_lexer_core.c src/game/ck2_lexer_core.h Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c17 -Wall -Wextra -Wpedantic -Werror -Isrc/game -o "$@" tests/event-core.c src/game/event_core.c src/game/ck2_lexer_core.c -lm

# Military and character lifecycle cores, independently testable without SDL.
.PHONY: check-warfare-core check-family-lifecycle
check-warfare-core: $(BUILD_DIR)/warfare-core-test
	"$(BUILD_DIR)/warfare-core-test"
$(BUILD_DIR)/warfare-core-test: tests/warfare-core.c src/game/warfare_core.c src/game/warfare_core.h Makefile
	@mkdir -p "$(BUILD_DIR)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG -std=c17 -Wall -Wextra -Wpedantic -Werror -o "$@" tests/warfare-core.c src/game/warfare_core.c
check-family-lifecycle: $(BUILD_DIR)/family-lifecycle-test
	"$(BUILD_DIR)/family-lifecycle-test"
$(BUILD_DIR)/family-lifecycle-test: tests/family-lifecycle.c src/game/family_gameplay.c src/game/family_gameplay.h src/game/economy_core.c Makefile
	@mkdir -p "$(BUILD_DIR)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG -std=c17 -Wall -Wextra -Wpedantic -Werror -Itests/stubs -Isrc/game -o "$@" tests/family-lifecycle.c src/game/family_gameplay.c src/game/economy_core.c -lm

.PHONY: check-warfare-map
check-warfare-map: $(BUILD_DIR)/warfare-map-test
	"$(BUILD_DIR)/warfare-map-test"
$(BUILD_DIR)/warfare-map-test: tests/warfare-map.c src/game/warfare_map.c src/game/warfare_core.c src/game/warfare_map.h src/game/warfare_core.h Makefile
	@mkdir -p "$(BUILD_DIR)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG -std=c17 -Wall -Wextra -Wpedantic -Werror -Itests/stubs -Isrc/game -o "$@" tests/warfare-map.c src/game/warfare_map.c src/game/warfare_core.c

.PHONY: check-console-warfare
check-console-warfare: $(BUILD_DIR)/console-warfare-test
	"$(BUILD_DIR)/console-warfare-test"
$(BUILD_DIR)/console-warfare-test: tests/console-warfare.c src/game/debug_console.c src/game/debug_console.h Makefile
	@mkdir -p "$(BUILD_DIR)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG -std=c17 -Wall -Wextra -Wpedantic -Werror -o "$@" tests/console-warfare.c src/game/debug_console.c

# In-game music skip selection (SDL-independent data test).
.PHONY: check-music-playlist
check-music-playlist: $(BUILD_DIR)/music-playlist-test
	"$(BUILD_DIR)/music-playlist-test"
$(BUILD_DIR)/music-playlist-test: tests/music-playlist-core.c src/game/music_playlist_core.c src/game/music_playlist_core.h Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c17 -Wall -Wextra -Wpedantic -Werror -o "$@" tests/music-playlist-core.c src/game/music_playlist_core.c $(LDLIBS)

.PHONY: check-bloodline-browser
check-bloodline-browser: $(BUILD_DIR)/bloodline-browser-test
	"$(BUILD_DIR)/bloodline-browser-test"
$(BUILD_DIR)/bloodline-browser-test: tests/bloodline-browser-core.c src/game/bloodline_browser_core.c src/game/bloodline_browser_core.h Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c17 -Wall -Wextra -Wpedantic -Werror -o "$@" tests/bloodline-browser-core.c src/game/bloodline_browser_core.c $(LDLIBS)

.PHONY: check-wonder-navigation
check-wonder-navigation: $(BUILD_DIR)/wonder-navigation-test
	"$(BUILD_DIR)/wonder-navigation-test"
$(BUILD_DIR)/wonder-navigation-test: tests/wonder-navigation-core.c src/game/wonder_navigation_core.c src/game/wonder_navigation_core.h Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c17 -Wall -Wextra -Wpedantic -Werror -o "$@" tests/wonder-navigation-core.c src/game/wonder_navigation_core.c $(LDLIBS)

.PHONY: check-bloodlines-core
check-bloodlines-core: $(BUILD_DIR)/bloodlines-core-test
	"$(BUILD_DIR)/bloodlines-core-test" $(CK2_BLOODLINE_TEST_FILES)
$(BUILD_DIR)/bloodlines-core-test: tests/bloodlines-core.c src/game/bloodline.c src/game/bloodline.h src/game/ck2_script.c Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG -std=c17 -Wall -Wextra -Wpedantic -Werror \
		-ffunction-sections -fdata-sections -Itests/stubs -Isrc/game -o "$@" \
		tests/bloodlines-core.c src/game/bloodline.c src/game/ck2_script.c -Wl,--gc-sections $(LDLIBS)

# CK2 laws: original-data parser + bounded hereditary succession; SDL-free.
.PHONY: check-law-core
check-law-core: $(BUILD_DIR)/law-core-test
	"$(BUILD_DIR)/law-core-test" $(CK2_LAW_TEST_FILES)
$(BUILD_DIR)/law-core-test: tests/law-core.c src/game/law_core.c src/game/law_core.h src/game/ck2_lexer_core.c src/game/ck2_lexer_core.h Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG -std=c17 -Wall -Wextra -Wpedantic -Werror -Isrc/game -o "$@" tests/law-core.c src/game/law_core.c src/game/ck2_lexer_core.c -lm

.PHONY: check-law-economy
check-law-economy: $(BUILD_DIR)/law-economy-test
	"$(BUILD_DIR)/law-economy-test"
$(BUILD_DIR)/law-economy-test: tests/law-economy.c src/game/law_core.c src/game/economy.c src/game/construction.c Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG -std=c17 -Wall -Wextra -Wpedantic -Werror -ffunction-sections -fdata-sections -Itests/stubs -Isrc/game $(LDFLAGS) \
		-Wl,--gc-sections -o "$@" tests/law-economy.c src/game/law_core.c src/game/ck2_lexer_core.c src/game/economy.c src/game/title_prestige.c src/game/construction.c src/game/levy_core.c src/game/ck2_script.c -lm $(LDLIBS)

.PHONY: check-realm-inheritance
check-realm-inheritance: $(BUILD_DIR)/realm-inheritance-test
	"$(BUILD_DIR)/realm-inheritance-test"
$(BUILD_DIR)/realm-inheritance-test: tests/realm-inheritance.c src/game/realms.c src/game/realms.h Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG -std=c17 -Wall -Wextra -Wpedantic -Werror -ffunction-sections -fdata-sections -Itests/stubs -Isrc/game -DSDL_FLIP_NONE=0 \
		-Wno-implicit-function-declaration -Wno-int-conversion $(LDFLAGS) -Wl,--gc-sections -o "$@" tests/realm-inheritance.c -lm $(LDLIBS)

# CK2 Byzantine/Roman imperial government: dated title-law integration.
# All fixtures are synthetic; real definitions are tested by check-governments-core.
.PHONY: check-byzantine-government
check-byzantine-government: $(BUILD_DIR)/byzantine-government-test
	"$(BUILD_DIR)/byzantine-government-test"
$(BUILD_DIR)/byzantine-government-test: tests/byzantine-government.c src/game/government_core.c src/game/law_core.c src/game/ck2_lexer_core.c src/game/government_core.h src/game/law_core.h Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG -std=c17 -Wall -Wextra -Wpedantic -Werror -Isrc/game -o "$@" tests/byzantine-government.c src/game/government_core.c src/game/law_core.c src/game/ck2_lexer_core.c -lm $(LDLIBS)

# CK2 authored succession-voting scripts. These are indexed for diagnostics;
# no dynamic voting conditions are treated as resolved or executed.
.PHONY: check-election-core
check-election-core: $(BUILD_DIR)/election-core-test
	"$(BUILD_DIR)/election-core-test" $(CK2_ELECTION_TEST_FILES)
$(BUILD_DIR)/election-core-test: tests/election-core.c src/game/election_core.c src/game/election_core.h src/game/law_core.c src/game/law_core.h src/game/ck2_lexer_core.c src/game/ck2_lexer_core.h Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG -std=c17 -Wall -Wextra -Wpedantic -Werror -Isrc/game -o "$@" tests/election-core.c src/game/election_core.c src/game/law_core.c src/game/ck2_lexer_core.c -lm $(LDLIBS)

# Native OpenCK2 checkpoint format (not CK2 save compatibility).
.PHONY: check-checkpoint-core
check-checkpoint-core: $(BUILD_DIR)/checkpoint-test
	"$(BUILD_DIR)/checkpoint-test"
$(BUILD_DIR)/checkpoint-test: tests/checkpoint-core.c src/game/checkpoint_core.c src/game/government_mechanics_core.c src/game/ck2_lexer_core.c src/game/checkpoint_core.h Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG -std=c17 -Wall -Wextra -Wpedantic -Werror -Itests/stubs -Isrc/game \
		$(LDFLAGS) -o "$@" tests/checkpoint-core.c src/game/checkpoint_core.c src/game/government_mechanics_core.c src/game/ck2_lexer_core.c $(LDLIBS)

.PHONY: check-construction-checkpoint
check-construction-checkpoint: $(BUILD_DIR)/construction-checkpoint-test
	"$(BUILD_DIR)/construction-checkpoint-test"
$(BUILD_DIR)/construction-checkpoint-test: tests/construction-checkpoint.c src/game/construction.c src/game/construction.h Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG -std=c17 -Wall -Wextra -Wpedantic -Werror -ffunction-sections -fdata-sections -Itests/stubs -Isrc/game \
		$(LDFLAGS) -Wl,--gc-sections -o "$@" tests/construction-checkpoint.c src/game/construction.c src/game/ck2_script.c -lm $(LDLIBS)

# Nomad, republic (including merchant), and theocracy government rules.
# All logic here is SDL independent. Pass the original government .txt paths
# with CK2_GOVERNMENT_TEST_FILES to test actual installed CK2/mod data.
.PHONY: check-nomad-republic-theocracy
check-nomad-republic-theocracy: $(BUILD_DIR)/nomad-republic-theocracy-test
	"$(BUILD_DIR)/nomad-republic-theocracy-test" $(CK2_GOVERNMENT_TEST_FILES)
$(BUILD_DIR)/nomad-republic-theocracy-test: tests/nomad-republic-theocracy.c src/game/government_core.c src/game/ck2_lexer_core.c src/game/government_core.h Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG -std=c17 -Wall -Wextra -Wpedantic -Werror -o "$@" tests/nomad-republic-theocracy.c src/game/government_core.c src/game/ck2_lexer_core.c $(LDLIBS)

# CK2 defines.lua / trade-post source driven gameplay core. SDL-independent.
.PHONY: check-government-mechanics
check-government-mechanics: $(BUILD_DIR)/government-mechanics-test
	"$(BUILD_DIR)/government-mechanics-test" $(CK2_DEFINES_TEST_FILE) $(CK2_TRADE_POST_TEST_FILE)
$(BUILD_DIR)/government-mechanics-test: tests/government-mechanics.c src/game/government_mechanics_core.c src/game/government_mechanics_core.h src/game/ck2_lexer_core.c
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG -std=c17 -Wall -Wextra -Wpedantic -Werror -Isrc/game -o "$@" tests/government-mechanics.c src/game/government_mechanics_core.c src/game/ck2_lexer_core.c -lm
