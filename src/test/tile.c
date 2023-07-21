#include "src/common.h"
#include "src/gfx/gfx.h"
#include "src/test/pattern.h"
#include <conio.h>

int main(int argc, char *argv[]) {
    int x = 0, y = 0, i = 0;
    bool to_blank = TRUE;
    gfx_buffer *tileset_buffer;
    gfx_tilemap *tilemap_buffer;
    byte *tile_ref;

    gfx_init();

    /*
     * the gfx system allocates 65536 bytes for a 256x256 tileset,
     * then loads it into VRAM in planar format.
     */
    tileset_buffer = gfx_get_tileset_buffer();
    render_pattern_to_buffer_1(tileset_buffer->buffer, TILE_WIDTH, TILE_HEIGHT, tileset_buffer->width, tileset_buffer->height);
    gfx_load_tileset();

    /*
     * the gfx system also allocates 65536 bytes for a 256x256 tile
     * map, which the system reads to know which tiles to scroll to.
     *
     * This is where you want to load your level's tiles.
     */
    tilemap_buffer = gfx_get_tilemap_buffer();

    for(i = 0; i < 256; i++){
        x = (i % 16) + 2;
        y = i / 16;
        tilemap_buffer->buffer[y * tilemap_buffer->horz_tiles + x] = (byte) i;

        /*
         * However, the tiles themselves first need to be initialzed
         * in the tilemap of both pages' screen states so that they
         * can be immediately rendered
         */
        gfx_set_tile((byte) i, x, y);
    }

    i = 0;
    while(!kbhit()) {
        if(i < 256) {
            x = (i % 16) + 2;
            y = i / 16;
            gfx_set_tile(to_blank ? 0 : i, x, y);
            i++;
        } else {
            i = 0;
            to_blank ^= 1;
        }

        gfx_render_all();
    }

    gfx_shutdown();

    return 0;
}
