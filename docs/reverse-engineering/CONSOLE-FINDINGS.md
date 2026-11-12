# Console System Investigation Findings

## Summary
The console is a developer/player command interface in CK2 that allows executing cheat commands, debugging tools, and game modifications. It is **disabled in Ironman mode** and accessed via the grave accent key (``).

## Key Access Method
- **Primary key**: Grave accent / backtick (ASCII 96, keycode SDLK_GRAVE)
- **Location**: Left of `1` on US keyboards
- **Alternative methods**: Various Alt+key combinations for different layouts
- **Ironman restriction**: Console is disabled when Ironman mode is enabled

## Implementation Requirements

### Missing Components in OpenCK2
1. **Console UI**: Text input field and output display area
2. **Command parser**: Lua command interpreter (`console_commands.lua`)
3. **Key handler**: `SDL_KEYDOWN` case for SDLK_GRAVE (currently not handled)
4. **Ironman flag**: Game setting to enable/disable console access
5. **Security layer**: Command whitelist/verification for safe execution

### Event Handler Location
Based on code analysis, console needs addition to:
- `src/game/main.c:2561` - Key handling switch statement (SDL_KEYDOWN case)
- Add case for `SDLK_GRAVE` to toggle console visibility
- Console screen state management in menu structure

### Core Commands Available
The original game's Lua console supports extensive commands:

**Character Commands** (`add_*`, `remove_*`, trait manipulation):
- `add_trait`, `remove_trait` - Trait modifications
- `add_modifier`, `remove_modifier` - Temporary effects
- `kill`, `die`, `age` - Character state changes
- `add_diplomacy`, `add_intrigue`, etc. - Attribute adjustments

**Global Commands**:
- `marry_anyone` - Marriage restrictions
- `discover_plots` - Plot visibility toggle
- `recalc_council` - Council recalculation (Conclave feature)

**Debug Commands** (Developer only):
- `debug_ai`, `reloadinterface`, `reload_succession_voting`
- Event and trigger testing with `testevent`

### Security Considerations
- Original CK2 marks all `debug_*` commands as developer-only
- Ironman mode prevents console access entirely
- Save file format difference between Ironman/Savegame (`.sav`) vs non-Ironman (plain text)

## Data Files Required
1. **Lua scripts**: `console_commands.lua`, `events/lua/console/*.lua`
2. **Game definitions**: `common/debug_*.txt`, command parameter lists
3. **Interface assets**: Console window graphics, input field textures

## Evidence from Original Game
- Console accessible via backtick key in non-Ironman saves
- Ironman save format blocks console execution at runtime
- Command scope system: Character/Global/None/Debug categories
- Parameter validation through Lua string operations

## Implementation Approach

### Phase 1: Basic Console Frame
1. Add console visibility flag to menu structure
2. Create SDL window overlay for console input/output
3. Handle `SDLK_GRAVE` keypress to show/hide console
4. Implement basic text rendering and input capture

### Phase 2: Command Interpreter
5. Create command parser for simple token-based execution
6. Implement character lookup via `charinfo` command
7. Basic parameter validation (title names, character IDs)

### Phase 3: Safe Subsets
8. Port non-destructive commands first (stats display, culture change)
9. Implement `help` command with documentation
10. Gradually add more complex operations with verification

## Related Project Files
- `src/game/main.c:2561` - Key event handling location (needs SDLK_GRAVE case added)
- `src/game/settings.c` - Settings structure
- `docs/UI-TODO.md` - Character panel related work  
- `docs/CK2-MECHANICS.md:44` - Notes on Conclave council system

## Status: NOT IMPLEMENTED - NEEDS CONSOLE SUBSYSTEM

## Recommended Implementation Order
1. Add case `SDLK_GRAVE` to switch at `main.c:2631`
2. Create console screen state in menu structure  
3. Implement basic console overlay with input field
4. Parse and validate simple commands
5. Port selected safe commands first
