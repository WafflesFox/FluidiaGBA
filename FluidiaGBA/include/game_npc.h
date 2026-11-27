#ifndef GAME_NPC_H
#define GAME_NPC_H

// ================================================================================================
// NPC table (npc_defs), temperaments, saved state, the live Npc struct, spawning
//
// Part of main.cpp. Include it ONLY from main.cpp, after main.cpp's own #includes and in the order
// listed there: it uses everything main.cpp has declared before it.
// ================================================================================================

namespace
{
    // TO ADD AN NPC:
    //  1. #include its sprite header at the top ("bn_sprite_items_<name>.h")
    //     (its sheet needs the same frame layout as the others, because first_frame() is shared)
    //  2. write its dialogue like snep_nodes above (an enum of node names, choice arrays, a nodes array)
    //  3. add one line to this table
    constexpr NpcDef npc_defs[] =
    {
        //  name       sprite                    map        x                        y                      wander    traits          nodes       start
        { "Filip:",  &bn::sprite_items::snep,  MAP_WORLD, world::pixel_w / 2 + 88, world::pixel_h / 2 + 8,  48 * T,   TRAIT_EXTROVERT, snep_nodes, SNEP_BABE, &snep_fighter},
        { "Waffles:", &bn::sprite_items::waffle, MAP_WORLD, world::pixel_w/2 + 50, world::pixel_h/2 + 10, 90 * T, TRAIT_PLAYFUL | TRAIT_INTROVERT, fox_nodes, FOX_CLONE, &waffle_fighter},
        { "Umbreon:", &bn::sprite_items::umbreon, MAP_WORLD2, world::pixel_w/2 + 50, world::pixel_h/2 + 30, 30 * T, TRAIT_LAZY | TRAIT_WALKER, umb_nodes, UMB_START, &umb_fighter},
        { "Harue: ", &bn::sprite_items::harue, MAP_HOUSE, 5*T, 7*T, 10*T, TRAIT_EXTROVERT, harue_nodes, HAR_START, &har_fighter },
    };
    constexpr int npc_count = sizeof(npc_defs) / sizeof(npc_defs[0]);

    // -------------------------------- PART 2: MACHINERY --------------------------------

    // What has to survive when an NPC's sprite is thrown away (every map change frees the sprites):
    // where they are, which map, whether they follow the player, and where their dialogue starts.
    struct Temperament
    {
        bn::fixed speed = 1;
        int wait_min = 40, wait_range = 80;    // frames standing still: wait_min + random(wait_range)
        int walk_min = 20, walk_range = 40;    // frames walking
        int run_chance = 0;                    // % chance that a walk is a run
    };

    Temperament temperament_of(int traits)
    {
        Temperament t;
        if(traits & TRAIT_LAZY)
        {
            t.speed *= bn::fixed(0.5);
            t.wait_min = 100; t.wait_range = 120;
            t.walk_min = 10;  t.walk_range = 20;
        }
        if(traits & TRAIT_SPEEDY)
        {
            t.speed *= bn::fixed(1.5);
            t.wait_min = 20; t.wait_range = 40;
        }
        if(traits & TRAIT_PLAYFUL)
        {
            t.wait_min = 10; t.wait_range = 30;
            t.walk_min = 40; t.walk_range = 60;
            t.run_chance = 30;
        }
        if(traits & TRAIT_WALKER)
        {
            t.wait_min = 0; t.wait_range = 1;        // range must be at least 1 (random_int(0) is bad)
            t.walk_min = 40; t.walk_range = 60;
        }
        return t;
    }

    struct NpcSave
    {
        int map;
        int x, y;
        int home_x, home_y;
        int start_node;
        bool following;
    };

    NpcSave npc_save[npc_count];

    void init_npc_saves()
    {
        for(int i = 0; i < npc_count; ++i)
        {
            const NpcDef& d = npc_defs[i];
            npc_save[i] = NpcSave{ d.map, d.x, d.y, d.x, d.y, d.start_node, false };
        }
    }

    constexpr int FOLLOW_GAP      = 24;   // a follower stops when it is this close to the player (pixels)
    constexpr int FOLLOW_RUN_FAR  = 64;   // farther than this (dx + dy) and the follower starts running...
    constexpr int FOLLOW_RUN_NEAR = 40;   // ...until it is closer than this again

    // The live state of an NPC that exists right now (only NPCs on the current map are created).
    struct Npc
    {
        int id;                        // index into npc_defs / npc_save
        const NpcDef* def;             // everything fixed about them: sprite sheet, dialogue...
        bn::sprite_ptr sprite;
        int map;                       // the map they are on right now
        bn::fixed x;                   // world position, same system as the player
        bn::fixed y;
        int home_x;                    // kind of like an anchor or something
        int home_y;
        int dir;
        int state;
        int timer;                     // frames left in the current state
        int anim_timer;
        int step;
        int shown;
        int start_node;                // where the next conversation begins
        bool following;                // true = walks after the player
        bool running;                  // followers run when the player gets too far

        Npc(int npc_id) :
            id(npc_id),
            def(&npc_defs[npc_id]),
            sprite(npc_defs[npc_id].sprite_item->create_sprite(0, 0, first_frame(DOWN, false))),
            map(npc_save[npc_id].map),
            x(npc_save[npc_id].x),
            y(npc_save[npc_id].y),
            home_x(npc_save[npc_id].home_x),
            home_y(npc_save[npc_id].home_y),
            dir(DOWN), state(NPC_WAIT), timer(60), anim_timer(0), step(0), shown(first_frame(DOWN, false)),
            start_node(npc_save[npc_id].start_node),
            following(npc_save[npc_id].following),
            running(false)
        {
            sprite.set_bg_priority(1);     // same layer as the player, so roofs and canopies cover them too
            sprite.set_visible(false);     // hidden until the first frame puts them in the right place
        }
    };

    // Throw away the NPCs of the old map (this frees their sprites) and create the ones standing on map_id.
    // Followers come with the player: they are moved to the player's position first.
    // Remember what every NPC is doing, then free all their sprites (and the video memory they hold).
    // Used while a battle takes over the screen. spawn_npcs() brings them back afterwards.
    void park_npcs(bn::ivector<Npc>& npcs)
    {
        for(const Npc& n : npcs)
        {
            NpcSave& s = npc_save[n.id];
            s.x = n.x.integer();
            s.y = n.y.integer();
            s.home_x = n.home_x;
            s.home_y = n.home_y;
            s.start_node = n.start_node;
            s.following = n.following;
        }
        npcs.clear();
    }

    void spawn_npcs(bn::ivector<Npc>& npcs, int map_id, int player_x, int player_y)
    {
        park_npcs(npcs);                             // remember what the old NPCs were doing, and free them

        for(int i = 0; i < npc_count; ++i)
        {
            if(npc_save[i].following)
            {
                npc_save[i].map = map_id;
                npc_save[i].x = player_x;
                npc_save[i].y = player_y;
            }
        }

        for(int i = 0; i < npc_count; ++i)
        {
            if(npc_save[i].map == map_id && !npcs.full()) { npcs.emplace_back(i); }
        }
    }
}

#endif
