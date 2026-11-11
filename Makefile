CFLAGS ?= -O2
BUILD_DIR ?= bin
LAUNCHER := $(BUILD_DIR)/ck2-launcher

.PHONY: all launcher
all: launcher
launcher: $(LAUNCHER)

$(LAUNCHER): src/launcher/main.c Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c17 -Wall -Wextra -Wpedantic $(LDFLAGS) -o "$@" "$<" $(LDLIBS)

STARTUP_TEST := $(BUILD_DIR)/launcher-startup-test

.PHONY: check-launcher-startup
check-launcher-startup: $(STARTUP_TEST)
	"$(STARTUP_TEST)"

$(STARTUP_TEST): src/launcher/startup.c src/launcher/startup.h tests/launcher-startup.c Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG -std=c17 -Wall -Wextra -Wpedantic -Isrc/launcher $(LDFLAGS) -o "$@" src/launcher/startup.c tests/launcher-startup.c $(LDLIBS)

CONTENT_TEST := $(BUILD_DIR)/launcher-content-test
CONTENT_SOURCES := src/launcher/content.c src/launcher/lifecycle.c tests/launcher-content.c

.PHONY: check-launcher check-launcher-content
check-launcher: check-launcher-startup check-launcher-content

check-launcher-content: $(CONTENT_TEST)
	"$(CONTENT_TEST)"

$(CONTENT_TEST): $(CONTENT_SOURCES) src/launcher/content.h src/launcher/lifecycle.h Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG -std=c17 -Wall -Wextra -Wpedantic -Isrc/launcher $(LDFLAGS) -o "$@" $(CONTENT_SOURCES) $(LDLIBS)

GAME := $(BUILD_DIR)/ck2-game
CONTENT_CATALOG := src/open_launcher/asset_paths.c src/open_launcher/zip_assets.c src/open_launcher/install.c src/open_launcher/catalog.c
CONTENT_HEADERS := src/open_launcher/zip_assets.h src/open_launcher/asset_paths.h src/open_launcher/install.h src/open_launcher/catalog.h
GAME_SOURCES := src/game/main.c src/game/assets.c src/game/music.c src/game/settings.c src/game/era.c src/game/portraits.c src/game/ck2_script.c src/game/rules.c src/game/realms.c src/game/holdings.c src/game/rulers.c src/game/campaign.c src/game/map_view.c src/game/models.c $(CONTENT_CATALOG)

.PHONY: game
all: game
game: open-launcher

$(GAME): $(GAME_SOURCES) src/game/assets.h src/game/music.h src/game/settings.h src/game/era.h src/game/portraits.h src/game/ck2_script.h src/game/rules.h src/game/realms.h src/game/holdings.h src/game/rulers.h src/game/campaign.h src/game/map_view.h src/game/models.h src/version.h $(CONTENT_HEADERS) Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c17 -Wall -Wextra -Wpedantic \
		$$(pkg-config --cflags sdl2 SDL2_image SDL2_mixer zlib) $(LDFLAGS) -o "$@" $(GAME_SOURCES) \
		$$(pkg-config --libs sdl2 SDL2_image SDL2_mixer zlib) -lm $(LDLIBS)

OPEN_LAUNCHER := $(BUILD_DIR)/OpenCK2Launcher
OPEN_LAUNCHER_SOURCES := src/open_launcher/main.c $(CONTENT_CATALOG) src/game/assets.c

.PHONY: open-launcher
all: open-launcher
open-launcher: $(OPEN_LAUNCHER) $(GAME)

ifneq ($(wildcard assets/openck2-background.png),)
open-launcher: $(BUILD_DIR)/openck2-background.png

$(BUILD_DIR)/openck2-background.png: assets/openck2-background.png
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
