CFLAGS ?= -O2
BUILD_DIR ?= bin

.PHONY: all
all: game

GAME := $(BUILD_DIR)/OpenCK2
CONTENT_CATALOG := src/open_launcher/asset_paths.c src/open_launcher/zip_assets.c src/open_launcher/install.c src/open_launcher/catalog.c
CONTENT_HEADERS := src/open_launcher/zip_assets.h src/open_launcher/asset_paths.h src/open_launcher/install.h src/open_launcher/catalog.h
GAME_SOURCES := src/game/main.c src/game/character_view.c src/game/assets.c src/game/music.c src/game/settings.c src/game/era.c src/game/portraits.c src/game/ck2_script.c src/game/rules.c src/game/realms.c src/game/holdings.c src/game/construction.c src/game/rulers.c src/game/campaign.c src/game/map_view.c src/game/models.c src/game/setup_toolbar.c src/game/frontend_gui.c src/game/debug_console.c $(CONTENT_CATALOG)

.PHONY: game
game: open-launcher

$(GAME): $(GAME_SOURCES) src/game/character_view.h src/game/assets.h src/game/music.h src/game/settings.h src/game/era.h src/game/portraits.h src/game/ck2_script.h src/game/rules.h src/game/realms.h src/game/holdings.h src/game/construction.h src/game/rulers.h src/game/campaign.h src/game/map_view.h src/game/models.h src/game/setup_toolbar.h src/game/frontend_gui.h src/game/debug_console.h src/version.h $(CONTENT_HEADERS) Makefile
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
TOOLBAR_TEST_SOURCES := tests/setup-toolbar.c src/game/setup_toolbar.c src/game/frontend_gui.c src/game/assets.c src/game/ck2_script.c $(CONTENT_CATALOG)

.PHONY: check-setup-toolbar
check-setup-toolbar: $(TOOLBAR_TEST)
	"$(TOOLBAR_TEST)"

$(TOOLBAR_TEST): $(TOOLBAR_TEST_SOURCES) src/game/setup_toolbar.h src/game/frontend_gui.h src/game/assets.h src/game/ck2_script.h $(CONTENT_HEADERS) Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG -std=c17 -Wall -Wextra -Wpedantic -Isrc/game -Isrc/open_launcher \
		$$(pkg-config --cflags sdl2 SDL2_image zlib) $(LDFLAGS) -o "$@" $(TOOLBAR_TEST_SOURCES) \
		$$(pkg-config --libs sdl2 SDL2_image zlib) $(LDLIBS)

HOLDING_SOURCES := src/game/holdings.c src/game/campaign.c src/game/ck2_script.c $(CONTENT_CATALOG)
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
PORTRAIT_TEST_SOURCES := tests/portraits.c src/game/assets.c src/game/ck2_script.c src/game/rulers.c src/game/portraits.c $(CONTENT_CATALOG)

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

$(CONSTRUCTION_TEST): tests/construction.c src/game/construction.c src/game/construction.h src/game/ck2_script.c $(CONTENT_CATALOG) $(CONTENT_HEADERS)
	mkdir -p "$(dir $@)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG -std=c17 -Wall -Wextra -Wpedantic -Isrc/game \
		$$(pkg-config --cflags sdl2 zlib) $(LDFLAGS) -o "$@" tests/construction.c src/game/construction.c src/game/ck2_script.c $(CONTENT_CATALOG) \
		$$(pkg-config --libs sdl2 zlib) $(LDLIBS)
