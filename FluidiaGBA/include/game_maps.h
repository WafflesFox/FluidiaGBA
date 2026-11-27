#ifndef GAME_MAPS_H
#define GAME_MAPS_H

// ================================================================================================
// Maps and warps: map ids, warp rectangles, walkability, names, map music
//
// Part of main.cpp. Include it ONLY from main.cpp, after main.cpp's own #includes and in the order
// listed there: it uses everything main.cpp has declared before it.
// ================================================================================================

namespace
{
    // ================================================================================
    // MAPS AND WARPS
    // ================================================================================

    constexpr int T = 16;           // one map tile, in pixels

    enum MapId { MAP_WORLD, MAP_WORLD2, MAP_HOUSE, MAP_WORLD3};

    // A warp is a rectangle on a map, in world pixels. When the player's FEET step inside it,
    // the game loads dest_map and puts the player's center at (dest_x, dest_y) there.
    // Always arrive OUTSIDE the destination's own warp rectangles, or you bounce straight back.
    struct Warp { int x, y, w, h; int dest_map; int dest_x, dest_y; };

    constexpr Warp world_warps[] =
    {
        // the house door (tile 41,41) -> inside the house, standing just above its door mat
        { 41 * T, 41 * T, T, T,                 MAP_HOUSE,  7 * T + 8, 11 * T },
        // the east edge of the world (a gap in the trees on rows 32-33) -> west side of world2
        { world::pixel_w - 8, 32 * T, 8, 2 * T, MAP_WORLD2, 3 * T,     32 * T + 8 },
        { 0, 32 * T, 8, 2 * T,                  MAP_WORLD3, world::pixel_w - 3 * T, 32 * T + 8},
    };
    constexpr Warp world3_warps[] =
    {
        {world::pixel_w - 8, 32 * T, 8, 2 * T, MAP_WORLD, 3 * T, 32 * T + 8},
    };

    constexpr Warp world2_warps[] =
    {
        // the west edge -> back to the east side of the first world
        { 0, 32 * T, 8, 2 * T,                  MAP_WORLD,  world::pixel_w - 3 * T, 32 * T + 8 },
    };

    constexpr Warp house_warps[] =
    {
        // the door mat (tile 7,12) -> outside, just below the house door
        { 7 * T, 12 * T, T, T,                  MAP_WORLD,  41 * T + 8, 42 * T },
    };

    // everything that differs per map lives in these small functions

    int map_width(int id)
    {
        switch(id)
        {
            case MAP_HOUSE:  return house_interior::pixel_w;
            case MAP_WORLD2: return world2::pixel_w;
            case MAP_WORLD3: return demo::pixel_w;
            default:         return world::pixel_w;
        }
    }

    int map_height(int id)
    {
        switch(id)
        {
            case MAP_HOUSE:  return house_interior::pixel_h;
            case MAP_WORLD2: return world2::pixel_h;
            case MAP_WORLD3: return demo::pixel_h;
            default:         return world::pixel_h;
        }
    }

    bool map_walkable(int id, int px, int py)
    {
        switch(id)
        {
            case MAP_HOUSE:  return house_interior::is_walkable(px, py);
            case MAP_WORLD2: return world2::is_walkable(px, py);
            case MAP_WORLD3: return demo::is_walkable(px, py);
            default:         return world::is_walkable(px, py);
        }
    }

    // the name shown in the corner (rename these to whatever you like)
    const char* map_name(int id)
    {
        switch(id)
        {
            case MAP_HOUSE:  return "Home";
            case MAP_WORLD2: return "Second World";
            case MAP_WORLD3: return "Terrain Test";
            default:         return "Grassy Plains";
        }
    }
    const MusicTrack& map_music(int id)
    {
        switch(id)
        {
            case MAP_WORLD:  return game_track;      // later: return house_track;
            case MAP_WORLD2: return game_track;
            case MAP_HOUSE: return house_track;
            case MAP_WORLD3: return game_track;
            default:         return game_track;
        }
    }
}

#endif
