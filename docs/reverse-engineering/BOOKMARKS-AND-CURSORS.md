# Custom Game Setup bookmarks and game cursors

Evidence from Steam CK2 3.3.5.1 and the OpenCK2 implementation.

## Bookmarks and scrollbar

`common/bookmarks/00_bookmarks.txt` has 11 dated records. Five carry
`era = yes` and supply the large era cards. The other six are additional
dates for the setup list:

| Bookmark | Date |
| --- | --- |
| Norman Conquest | 26 December 1066 |
| Komnenos Dynasty | 1 April 1081 |
| Third Crusade | 1 January 1187 |
| The Latin Empire | 16 May 1204 |
| Age of the Mongols | 1 February 1220 |
| Rise of the Hansa | 1 May 1241 |

The screenshot shows the final four rows of the list, with Late Middle
Ages (1 January 1337) following the last three extra dates. The
`bookmarks_list` in `interface/frontend.gui` is 168 logical pixels
tall, and `bookmark_entry` is 42 pixels high. Four rows fit.
The list requests `standardlistbox_slider` from
`interface/EU3_core.gui`. The Interesting Characters list has its own
scroll position. The screenshot is consistent with scrolling dates
while the selected character list stays on Otto's bookmark.

OpenCK2 reads all 11 records for Custom Game Setup and keeps the
five era-tagged cards in the era picker. Setup uses the same CK2
scrollbar art as the vassal list. Wheel, arrows, track clicks, and
thumb dragging change the list offset independently of selection.
Selecting a bookmark reloads the historical snapshot and clears the
previous ruler selection. Leaving setup on a bookmark without an
era card returns to the card picker.

Native DLC-dependent bookmark availability and exact scroll behavior
remain to be verified against matching profiles.

## Cursor

The installed game ships pointer artwork in `gfx/cursors/`.
`normal.png` is a 32 × 32 RGBA image. The decoded `normal.cur` has
identical RGBA pixels and a hotspot of (0, 0). The pinned Windows
executable contains `/cursors/normal.cur` at VA `0x14101f118`
and references it at `0x1403857d2`. It also references busy and
drag-select cursor assets and imports `LoadCursorFromFileA`,
`SetCursor`, and `SDL_SetCursor`. This establishes a game-supplied
cursor path. Exact state transitions and native platform-specific
selection remain unverified. OpenCK2 loads the installed `normal.png` into an SDL color cursor
with the original (0, 0) hotspot. The default cursor remains available
if loading or cursor creation fails and is restored on window leave,
focus loss, and shutdown.

Busy, drag-select, and movement cursors remain pending. Observe
vanilla's state transitions before adding them. Animated `.ani`
decoding and timing are separate from the static cursor.

## Verification

On Linux, dragging the bookmark thumb to the bottom exposed The Latin
Empire, Age of the Mongols, Rise of the Hansa, and Late Middle Ages.
Selecting The Latin Empire changed the date to 16 May 1204, populated
its featured characters from dated history, refreshed the description
and Realms map, and opened Game Rules through Play. The X11 cursor
query matched the installed cursor's 365 opaque pixels, 32 × 32
dimensions, and (0, 0) hotspot. High-DPI and other platform backends
remain unverified.
