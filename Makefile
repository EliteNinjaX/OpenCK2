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
GAME_SOURCES := src/game/main.c src/game/assets.c src/game/music.c

.PHONY: game
all: game
game: $(GAME)

$(GAME): $(GAME_SOURCES) src/game/assets.h src/game/music.h Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c17 -Wall -Wextra -Wpedantic \
		$$(pkg-config --cflags sdl2 SDL2_image SDL2_mixer) $(LDFLAGS) -o "$@" $(GAME_SOURCES) \
		$$(pkg-config --libs sdl2 SDL2_image SDL2_mixer) $(LDLIBS)

OPEN_LAUNCHER := $(BUILD_DIR)/OpenCK2Launcher
OPEN_LAUNCHER_SOURCES := src/open_launcher/main.c src/open_launcher/install.c src/open_launcher/catalog.c src/game/assets.c

.PHONY: open-launcher
all: open-launcher
open-launcher: $(OPEN_LAUNCHER) $(GAME)

ifneq ($(wildcard assets/openck2-background.png),)
open-launcher: $(BUILD_DIR)/openck2-background.png

$(BUILD_DIR)/openck2-background.png: assets/openck2-background.png
	mkdir -p "$(@D)"
	cp "$<" "$@"
endif

$(OPEN_LAUNCHER): $(OPEN_LAUNCHER_SOURCES) src/open_launcher/install.h src/open_launcher/catalog.h src/game/assets.h Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c17 -Wall -Wextra -Wpedantic \
		$$(pkg-config --cflags sdl2 SDL2_image) $(LDFLAGS) -o "$@" $(OPEN_LAUNCHER_SOURCES) \
		$$(pkg-config --libs sdl2 SDL2_image) $(LDLIBS)
