#ifndef GAME_MAP_GFX_H
#define GAME_MAP_GFX_H

// ================================================================================================
// Creating the ground / top layers of a map
//
// Part of main.cpp. Include it ONLY from main.cpp, after main.cpp's own #includes and in the order
// listed there: it uses everything main.cpp has declared before it.
// ================================================================================================

namespace
{
    bn::regular_bg_ptr make_ground(int id)
    {
        switch(id)
        {
            case MAP_HOUSE:  return bn::regular_bg_items::house_interior.create_bg(0, 0);
            case MAP_WORLD2: return bn::regular_bg_items::world2.create_bg(0, 0);
            case MAP_WORLD3: return bn::regular_bg_items::demo.create_bg(0, 0);
            default:         return bn::regular_bg_items::world.create_bg(0, 0);
        }
    }

    // the layer drawn in front of the player (roofs, canopies); not every map has one
    bn::optional<bn::regular_bg_ptr> make_top(int id)
    {
        if(id == MAP_WORLD)
        {
            return bn::regular_bg_items::world_top.create_bg(0, 0);
        } else if (id == MAP_WORLD3){
            return bn::regular_bg_items::demo_top.create_bg(0, 0);
        }
        return bn::optional<bn::regular_bg_ptr>();
    }

    const Warp* map_warps(int id, int& count)
    {
        switch(id)
        {
            case MAP_HOUSE:
                count = sizeof(house_warps) / sizeof(house_warps[0]);
                return house_warps;
            case MAP_WORLD2:
                count = sizeof(world2_warps) / sizeof(world2_warps[0]);
                return world2_warps;
            case MAP_WORLD3:
                count = sizeof(world3_warps) / sizeof(world3_warps[0]);
                return world3_warps;
            default:
                count = sizeof(world_warps) / sizeof(world_warps[0]);
                return world_warps;
        }
    }
}

#endif
