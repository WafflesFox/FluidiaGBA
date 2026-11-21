#define GAME_VER "0.2.9p"
#include "bn_core.h"
#include "bn_color.h"
#include "bn_bg_palettes.h"
#include "bn_fixed.h"
#include "bn_keypad.h"
#include "bn_optional.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"
#include "bn_string.h"
#include "common_variable_8x16_sprite_font.h"
#include "bn_sprite_items_player.h"
#include "bn_sprite_affine_mat_ptr.h"
#include "bn_regular_bg_ptr.h"
#include "bn_regular_bg_items_layer_2_mountains.h"
#include "bn_regular_bg_items_layer_3_hills.h"
#include "bn_regular_bg_items_layer_4_forest.h"
#include "bn_sound_items.h"
#include "bn_sound_item.h"
#include "bn_sound_handle.h"

//characters
#include "bn_sprite_items_waffle.h"
#include "bn_sprite_items_snep.h"
#include "bn_sprite_items_umbreon.h"

#include "bn_sprite_items_black_block.h"   // solid black 64x64, used by the screen transition

// the maps: ground picture, the layer drawn in front of the player, and the collision grids
#include "bn_regular_bg_items_world.h"
#include "bn_regular_bg_items_world_top.h"
#include "bn_regular_bg_items_world2.h"
#include "bn_regular_bg_items_house_interior.h"
#include "world_map.h"              // namespace world
#include "world2_map.h"             // namespace world2
#include "house_interior_map.h"     // namespace house_interior



namespace
{
    constexpr int music_chunks = 5;   // how many chunks
    constexpr int walk_cycle[4] = {0,1,0,2}; // the animation >w<
    enum Dir {UP = 0, DOWN = 1, LEFT = 2, RIGHT = 3};

    int first_frame(int dir, bool running)
    {
        return (running ? 12 : 0) + dir * 3;
    }

    bn::fixed clamp_fixed(bn::fixed v, bn::fixed lo, bn::fixed hi)
    {
        if(v < lo) { return lo; }
        if(v > hi) { return hi; }
        return v;
    }

    bn::sound_item music_chunk(int index) // title music
    {
        switch(index)
        {
            case 0:  return bn::sound_items::title_00;
            case 1:  return bn::sound_items::title_01;
            case 2:  return bn::sound_items::title_02;
            case 3: return bn::sound_items::title_03;
            default: return bn::sound_items::title_04;
        }
    }
    bn::sound_item gameMusic_chunk(int index) // game music
    {
        switch(index){
            case 0: return bn::sound_items::game_00;
            case 1: return bn::sound_items::game_01;
            case 2: return bn::sound_items::game_02;
            case 3: return bn::sound_items::game_03;
            default: return bn::sound_items::game_04;
        }
    }

    void enlarge(bn::ivector<bn::sprite_ptr>& sprites, bn::fixed center_y, bn::fixed scale) // this function was helped made by Claude AI
    {
        bn::sprite_affine_mat_ptr mat = bn::sprite_affine_mat_ptr::create();
        mat.set_scale(scale);

        for(bn::sprite_ptr& s : sprites)
        {
            s.set_affine_mat(mat);                                          // one shared transform
            // s.set_double_size_mode(bn::sprite_double_size_mode::ENABLED);   // stops the edges getting clipped
            s.set_x(s.x() * scale);                                         // spread the letters out
            s.set_y(center_y + (s.y() - center_y) * scale);
        }
    }

    // ================================================================================
    // MAPS AND WARPS
    // ================================================================================

    constexpr int T = 16;           // one map tile, in pixels

    enum MapId { MAP_WORLD, MAP_WORLD2, MAP_HOUSE };

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
            default:         return world::pixel_w;
        }
    }

    int map_height(int id)
    {
        switch(id)
        {
            case MAP_HOUSE:  return house_interior::pixel_h;
            case MAP_WORLD2: return world2::pixel_h;
            default:         return world::pixel_h;
        }
    }

    bool map_walkable(int id, int px, int py)
    {
        switch(id)
        {
            case MAP_HOUSE:  return house_interior::is_walkable(px, py);
            case MAP_WORLD2: return world2::is_walkable(px, py);
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
            default:         return "Grassy Plains";
        }
    }

    bn::regular_bg_ptr make_ground(int id)
    {
        switch(id)
        {
            case MAP_HOUSE:  return bn::regular_bg_items::house_interior.create_bg(0, 0);
            case MAP_WORLD2: return bn::regular_bg_items::world2.create_bg(0, 0);
            default:         return bn::regular_bg_items::world.create_bg(0, 0);
        }
    }

    // the layer drawn in front of the player (roofs, canopies); not every map has one
    bn::optional<bn::regular_bg_ptr> make_top(int id)
    {
        if(id == MAP_WORLD)
        {
            return bn::regular_bg_items::world_top.create_bg(0, 0);
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
            default:
                count = sizeof(world_warps) / sizeof(world_warps[0]);
                return world_warps;
        }
    }

    // ================================================================================
    // SCREEN TRANSITION: a black block slides across the screen
    // ================================================================================

    // The block is a 4x3 grid of 64x64 black sprites = 256x192 pixels, a bit bigger than the 240x160 screen.
    constexpr int TRANSITION_SPEED = 10;   // pixels per frame (higher = faster)
    constexpr int BLOCK_COLS = 4;
    constexpr int BLOCK_ROWS = 3;

    // how far the block's center starts from the screen's center, so that it begins just off-screen
    int transition_distance(int dir)
    {
        return (dir == LEFT || dir == RIGHT) ? 248 : 176;     // (240 + 256) / 2   and   (160 + 192) / 2
    }

    void make_blocks(bn::vector<bn::sprite_ptr, 12>& blocks)
    {
        for(int row = 0; row < BLOCK_ROWS; ++row)
        {
            for(int col = 0; col < BLOCK_COLS; ++col)
            {
                bn::sprite_ptr s = bn::sprite_items::black_block.create_sprite(
                        bn::fixed(-96 + col * 64), bn::fixed(-64 + row * 64));
                s.set_bg_priority(0);        // in front of the backgrounds...
                s.set_z_order(-32000);       // ...and in front of every other sprite (lowest z order is drawn last)
                blocks.push_back(s);
            }
        }
    }

    // trans_pos is where the block is along its travel axis: it counts down from +distance to -distance,
    // and 0 means the block covers the whole screen. The facing direction decides which side it comes from:
    // facing RIGHT it enters from the right and slides left, facing LEFT it comes from the left, and so on.
    void move_blocks(bn::vector<bn::sprite_ptr, 12>& blocks, int dir, bn::fixed trans_pos)
    {
        bn::fixed off_x = 0;
        bn::fixed off_y = 0;
        switch(dir)
        {
            case RIGHT: off_x = trans_pos;  break;
            case LEFT:  off_x = -trans_pos; break;
            case DOWN:  off_y = trans_pos;  break;
            default:    off_y = -trans_pos; break;     // UP
        }

        for(int i = 0; i < blocks.size(); ++i)
        {
            const int col = i % BLOCK_COLS;
            const int row = i / BLOCK_COLS;
            blocks[i].set_position(bn::fixed(-96 + col * 64) + off_x, bn::fixed(-64 + row * 64) + off_y);
        }
    }
    constexpr int FEET = 12;           // same "feet" offset the player uses for collision

    int abs_int(int v) { return v < 0 ? -v : v; }

    // A tiny random number generator (xorshift). Wandering only needs "random enough".
    unsigned rng_state = 2463534242u;

    int random_int(int limit)          // returns 0 .. limit-1
    {
        rng_state ^= rng_state << 13;
        rng_state ^= rng_state >> 17;
        rng_state ^= rng_state << 5;
        return int(rng_state % unsigned(limit));
    }

    enum NpcState { NPC_WAIT, NPC_WALK };

    // ================================================================================
    // NPCS
    // PART 1 is the DATA you edit: dialogue + the NPC table.
    // PART 2 is the machinery that runs them. Adding an NPC should never need PART 2.
    // ================================================================================

    // -------------------------------- PART 1: DATA --------------------------------

    constexpr int DLG_END  = -1;     // use as a "next" to close the box
    constexpr int DLG_KEEP = -1;     // use as "set_start" to leave the NPC's starting node alone

    enum { ACT_NONE, ACT_FOLLOW, ACT_STOP_FOLLOW};   // things a dialogue node can make the NPC do

    enum //traits
    {
        TRAIT_LAZY      = 1 << 0,
        TRAIT_SPEEDY    = 1 << 1,
        TRAIT_INTROVERT = 1 << 2,
        TRAIT_EXTROVERT = 1 << 3,
        TRAIT_WALKER    = 1 << 4,
        TRAIT_PLAYFUL   = 1 << 5,
    };

    // one answer the player can pick
    struct Choice
    {
        const char* text;            // keep it short (about 10 characters if there are 3 answers)
        int next;                    // node to jump to when picked (DLG_END closes the box)
    };

    // one box of dialogue. A conversation is a set of these that point at each other by index.
    struct DialogueNode
    {
        const char* text;            // what the NPC says (about 28 characters max)
        int next;                    // node after this one when A is pressed (DLG_END closes the box)
        const Choice* choices;       // nullptr = plain line; otherwise the player picks (LEFT/RIGHT, then A)
        int choice_count;
        int action;                  // ACT_... happens the moment this node appears
        int set_start;               // >= 0: the next time you talk to this NPC, begin at this node instead
    };

    // a plain line:  say("text", next_node, action, new_start_node)   (everything after the text is optional)
    constexpr DialogueNode say(const char* text, int next = DLG_END, int action = ACT_NONE, int set_start = DLG_KEEP)
    {
        return DialogueNode{ text, next, nullptr, 0, action, set_start };
    }

    // a question:  ask("text", the_choices_array)
    template<int N>
    constexpr DialogueNode ask(const char* text, const Choice (&choices)[N])
    {
        return DialogueNode{ text, DLG_END, choices, N, ACT_NONE, DLG_KEEP };
    }

    // The fixed facts about an NPC. These live in a constexpr table, so they cost no RAM.
    struct NpcDef
    {
        const char* name;                    // shown above the dialogue (include the ":" yourself)
        const bn::sprite_item* sprite_item;  // every NPC can use its own sprite sheet
        int map;                             // which map they start on (MAP_WORLD, MAP_HOUSE, ...)
        int x, y;                            // where they start, in world pixels
        int wander_radius;                   // how far from home they wander; 0 = stands still
        int traits;
        const DialogueNode* nodes;           // their dialogue
        int start_node;                      // the node a conversation begins with (nodes can change it later)
    };

    // ---- Snep's dialogue ----
    // Give every node a name here so the tables below stay readable. The ORDER must match snep_nodes[].
    enum SnepNode
    {
        SNEP_BABE, SNEP_TRAPPED, SNEP_LATER, SNEP_ALONE, SNEP_DOTS, SNEP_UPLOAD, SNEP_FOOTAGE,
        SNEP_OKAY, SNEP_OKAY_YES, SNEP_OKAY_NO,
        SNEP_MENU,
        SNEP_FOLLOW_Q, SNEP_FOLLOW_YES, SNEP_CONCERN,SNEP_FOLLOW_NO,
        SNEP_STOP_Q, SNEP_STOP_YES, SNEP_STOP_NO,
    };

    enum FoxNode
    {
        FOX_CLONE, FOX_WOAH, FOX_CONFUSE,
        FOX_MENU, FOX_YES, FOX_NO,
        FOX_STOP, FOX_STOP_YES, FOX_STOP_NO,
    };

    enum UmbNode
    {
        UMB_START, UMB_GREET, UMB_MAGICAL,
        UMB_REST,
        UMB_RECOMMEND, UMB_RECOMMEND_2, UMB_YES, UMB_NO,
        UMB_STOP_Q, UMB_STOP_Y, UMB_STOP_N,
        UMB_RECOMMEND3,
    };

    constexpr Choice snep_okay_choices[]   = { { "Yes", SNEP_OKAY_YES }, { "Not really", SNEP_OKAY_NO } };
    constexpr Choice snep_menu_choices[]   = { { "Follow me", SNEP_FOLLOW_Q }, { "Nothing", DLG_END } };
    constexpr Choice snep_follow_choices[] = { { "Yes", SNEP_FOLLOW_YES }, { "No", SNEP_FOLLOW_NO } };
    constexpr Choice snep_stop_choices[]   = { { "Wait here", SNEP_STOP_YES }, { "Keep going", SNEP_STOP_NO } };
    
    constexpr Choice fox_follow_choices[] = {{"Yes", FOX_YES}, {"No", FOX_NO}};
    constexpr Choice fox_stop_choices[] = {{"Yes", FOX_STOP_YES}, {"No", FOX_STOP_NO}};

    constexpr Choice umb_follow_choices[] = {{"Yes", UMB_YES}, {"No", UMB_NO}};
    constexpr Choice umb_stop_choices[] = {{"Right here", UMB_STOP_Y}, {"Not yet", UMB_STOP_N}};

    constexpr DialogueNode snep_nodes[] =
    {
        /* SNEP_BABE       */ say("Babe.", SNEP_TRAPPED),
        /* SNEP_TRAPPED    */ say("Why did you trap me in a game?", SNEP_LATER),
        /* SNEP_LATER      */ say("Im so gonna date you later.", SNEP_ALONE),
        /* SNEP_ALONE      */ say("No one can see us here right?", SNEP_DOTS),
        /* SNEP_DOTS       */ say("...", SNEP_UPLOAD),
        /* SNEP_UPLOAD     */ say("Unless you upload the...", SNEP_FOOTAGE),
        /* SNEP_FOOTAGE    */ say("footage of this game.", SNEP_OKAY),
        /* SNEP_OKAY       */ ask("Are you okay?", snep_okay_choices),
        // after either answer, next time you talk to him he starts at SNEP_MENU instead of the intro
        /* SNEP_OKAY_YES   */ say("Good. I was worried.", DLG_END, ACT_NONE, SNEP_MENU),
        /* SNEP_OKAY_NO    */ say("Aw... I'm here for you.", DLG_END, ACT_NONE, SNEP_MENU),
        /* SNEP_MENU       */ ask("Need anything?", snep_menu_choices),
        /* SNEP_FOLLOW_Q   */ ask("Huh, want me to follow you?", snep_follow_choices),
        // ACT_FOLLOW turns following on; set_start makes the NEXT talk begin with "want me to wait here?"
        /* SNEP_FOLLOW_YES */ say("Okay! You can run", SNEP_CONCERN),
                              say("Im sure I can keep up!", DLG_END, ACT_FOLLOW, SNEP_STOP_Q),
        /* SNEP_FOLLOW_NO  */ say("Alright, I'll stay here."),
        /* SNEP_STOP_Q     */ ask("Want me to wait here?", snep_stop_choices),
        /* SNEP_STOP_YES   */ say("Okay, I'll wait here.", DLG_END, ACT_STOP_FOLLOW, SNEP_MENU),
        /* SNEP_STOP_NO    */ say("Okay, lets go!"),
    };

    constexpr DialogueNode fox_nodes[]=
    {
        say("Woah", FOX_WOAH),
        say("You're Me! I'm You!", FOX_CONFUSE),
        say("Well Hi there cutie!", DLG_END, ACT_NONE, FOX_MENU),
        ask("Huh, want me to follow you?", fox_follow_choices),
        say("Oh, Okay. Lets go :3", DLG_END, ACT_FOLLOW, FOX_STOP ),
        say("Oh, uhm okay then,"),
        ask("Want me to stop following you?",fox_stop_choices),
        say("Okay then, Unfollowed!", DLG_END, ACT_STOP_FOLLOW, FOX_MENU),
        say("Okay, lets gooo uwu!"),

    };
    constexpr DialogueNode umb_nodes[]=
    {
        say("Wsg Waffles owo", UMB_GREET),
        say("I'm just chilling in this forest", UMB_MAGICAL),
        say("I'm thinking of practicing my Dark Magic", DLG_END, ACT_NONE, UMB_REST),
        say("Still working on it owo", DLG_END, ACT_NONE, UMB_RECOMMEND),
        say("Do you know a better place for me", UMB_RECOMMEND_2),
        ask("to practice my dark magic?", umb_follow_choices),
        say("Alright owo, show me", DLG_END, ACT_FOLLOW, UMB_STOP_Q),
        say("Aww, alright then."),
        ask("So... Here?", umb_stop_choices),
        say("Alright, Thanks!", DLG_END, ACT_STOP_FOLLOW, UMB_RECOMMEND3),
        say("Oooo Alr"),
        say("Yeah this is better.. but", UMB_RECOMMEND),
    };

    // TO ADD AN NPC:
    //  1. #include its sprite header at the top ("bn_sprite_items_<name>.h")
    //     (its sheet needs the same frame layout as the others, because first_frame() is shared)
    //  2. write its dialogue like snep_nodes above (an enum of node names, choice arrays, a nodes array)
    //  3. add one line to this table
    constexpr NpcDef npc_defs[] =
    {
        //  name       sprite                    map        x                        y                     wander    traits          nodes       start
        { "Filip:",  &bn::sprite_items::snep,  MAP_WORLD, world::pixel_w / 2 + 88, world::pixel_h / 2 + 8,  48 * T,   TRAIT_EXTROVERT, snep_nodes, SNEP_BABE },
        { "Waffles:", &bn::sprite_items::waffle, MAP_WORLD, world::pixel_w/2 + 50, world::pixel_h/2 + 10, 90 * T, TRAIT_PLAYFUL | TRAIT_INTROVERT, fox_nodes, FOX_CLONE},
        { "Umbreon:", &bn::sprite_items::umbreon, MAP_WORLD2, world::pixel_w/2 + 50, world::pixel_h/2 + 30, 30 * T, TRAIT_WALKER | TRAIT_PLAYFUL, umb_nodes, UMB_START},
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
        int home_x;                    // wanderers never go far from here
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
    void spawn_npcs(bn::ivector<Npc>& npcs, int map_id, int player_x, int player_y)
    {
        for(const Npc& n : npcs)                     // remember what the old NPCs were doing
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

    bool boxes_touch(int ax, int ay, int bx, int by)
    {
        return abs_int(ax - bx) < 16 && abs_int(ay - by) < 12;
    }

    // Does stepping from (cx, cy) to (nx, ny) run INTO the thing at (ox, oy)?
    // If you already overlap it you are free to walk away, so nobody can ever get stuck inside someone.
    bool bumps(int cx, int cy, int nx, int ny, int ox, int oy)
    {
        return boxes_touch(nx, ny, ox, oy) && !boxes_touch(cx, cy, ox, oy);
    }

    // would the player, stepping from (cx, cy) to (nx, ny), run into an NPC?
    bool blocked_by_npc(const bn::ivector<Npc>& npcs, int cx, int cy, int nx, int ny)
    {
        for(const Npc& n : npcs)
        {
            if(bumps(cx, cy, nx, ny, n.x.integer(), n.y.integer())) { return true; }
        }
        return false;
    }

    // the closest NPC the player is near enough to talk to, or nullptr
    Npc* npc_in_reach(bn::ivector<Npc>& npcs, int player_x, int player_y)
    {
        Npc* best = nullptr;
        int best_dist = 0;
        for(Npc& n : npcs)
        {
            const int dx = abs_int(player_x - n.x.integer());
            const int dy = abs_int(player_y - n.y.integer());
            if(dx < 28 && dy < 28 && (best == nullptr || dx + dy < best_dist))
            {
                best = &n;
                best_dist = dx + dy;
            }
        }
        return best;
    }

    // Try to move an NPC one step in a direction. Returns false (and does nothing) if something is in the way.
    // assumes UP, DOWN, LEFT, RIGHT = 0, 1, 2, 3, so "dir ^ 1" is the opposite direction
    int pick_wander_dir(const Npc& npc, int player_x, int player_y)
    {
        const int dx = player_x - npc.x.integer();
        const int dy = player_y - npc.y.integer();
        const int dist = abs_int(dx) + abs_int(dy);
        const int toward = abs_int(dx) > abs_int(dy) ? (dx < 0 ? LEFT : RIGHT) : (dy < 0 ? UP : DOWN);
        const int traits = npc.def->traits;

        if((traits & TRAIT_INTROVERT) && dist < 80 && random_int(100) < 70) { return toward ^ 1; }  // shy: walk away
        if((traits & TRAIT_EXTROVERT) && dist > 48 && random_int(100) < 65) { return toward; }      // drift closer
        return random_int(4);
    }
    bool npc_step(Npc& npc, const bn::ivector<Npc>& all_npcs, int dir, bn::fixed speed,
                  int player_x, int player_y, bool stay_near_home)
    {
        bn::fixed next_x = npc.x;
        bn::fixed next_y = npc.y;
        switch(dir)
        {
            case UP:    next_y -= speed; break;
            case DOWN:  next_y += speed; break;
            case LEFT:  next_x -= speed; break;
            default:    next_x += speed; break;
        }

        const int cx = npc.x.integer();
        const int cy = npc.y.integer();
        const int nx = next_x.integer();
        const int ny = next_y.integer();

        if(!map_walkable(npc.map, nx, ny + FEET)) { return false; }

        if(stay_near_home)
        {
            const int radius = npc.def->wander_radius;
            if(abs_int(nx - npc.home_x) > radius || abs_int(ny - npc.home_y) > radius) { return false; }
        }

        if(bumps(cx, cy, nx, ny, player_x, player_y)) { return false; }

        for(const Npc& other : all_npcs)                  // don't walk into the other NPCs either
        {
            if(&other != &npc && bumps(cx, cy, nx, ny, other.x.integer(), other.y.integer())) { return false; }
        }

        npc.x = next_x;
        npc.y = next_y;
        npc.dir = dir;
        return true;
    }
    void begin_walk(Npc& npc, int player_x, int player_y)
    {
        const Temperament t = temperament_of(npc.def->traits);
        npc.dir = pick_wander_dir(npc, player_x, player_y);
        npc.state = NPC_WALK;
        npc.timer = t.walk_min + random_int(t.walk_range);
        npc.running = random_int(100) < t.run_chance;     // playful NPCs sometimes run this walk
    }

    void begin_wait(Npc& npc)
    {
        const Temperament t = temperament_of(npc.def->traits);
        npc.state = NPC_WAIT;
        npc.timer = t.wait_min + random_int(t.wait_range);
        npc.running = false;

    }
    void update_npc(Npc& npc, const bn::ivector<Npc>& all_npcs, int player_x, int player_y)
    {
        bool moving = false;

        if(npc.following)
        {
            // ---- follow the player ----
            const int dx = player_x - npc.x.integer();
            const int dy = player_y - npc.y.integer();
            const int adx = abs_int(dx);
            const int ady = abs_int(dy);

            // too far away? run. (two different limits so they don't flicker between walking and running)
            const int farthest = adx > ady ? adx : ady;

            if(farthest > FOLLOW_RUN_FAR)       { npc.running = true; }
            else if(farthest < FOLLOW_RUN_NEAR) { npc.running = false; }

            if(adx >= FOLLOW_GAP || ady >= FOLLOW_GAP)    // not close enough yet
            {
                const bn::fixed speed = npc.running ? 1.75 : 1;   // runs a bit faster than the player so they catch up

                // go along the axis that is farther away; keep the current axis unless the other one is clearly bigger
                const bool horizontal_first = (npc.dir == LEFT || npc.dir == RIGHT) ? (adx + 6 > ady) : (adx > ady + 6);
                const int dir_x = dx < 0 ? LEFT : RIGHT;
                const int dir_y = dy < 0 ? UP : DOWN;

                moving = npc_step(npc, all_npcs, horizontal_first ? dir_x : dir_y, speed, player_x, player_y, false);
                if(!moving)    // blocked: try the other axis, this slides them around corners
                {
                    moving = npc_step(npc, all_npcs, horizontal_first ? dir_y : dir_x, speed, player_x, player_y, false);
                }
            }
        }
        else if(npc.def->wander_radius > 0)
        {
            const bool walker  = npc.def->traits & TRAIT_WALKER;
            const bool leashed = !(npc.def->traits & TRAIT_EXTROVERT);

            if(npc.state == NPC_WAIT)
            {
                if(--npc.timer <= 0) { begin_walk(npc, player_x, player_y); }
            }
            else
            {
                const Temperament t = temperament_of(npc.def->traits);
                const bn::fixed speed = npc.running ? t.speed * bn::fixed(1.75) : t.speed;

                moving = npc_step(npc, all_npcs, npc.dir, speed, player_x, player_y, leashed);

                if(!moving && walker)
                {
                    // walkers never stand around: bounce off and try other directions in the same frame
                    for(int tries = 0; tries < 4 && !moving; ++tries)
                    {
                        begin_walk(npc, player_x, player_y);
                        moving = npc_step(npc, all_npcs, npc.dir, speed, player_x, player_y, leashed);
                    }
                }
                else if(!moving)
                {
                    begin_wait(npc);                          // blocked: stop and think
                }

                if(moving && --npc.timer <= 0)
                {
                    if(walker) { begin_walk(npc, player_x, player_y); }
                    else       { begin_wait(npc); }
                }
            }
        }
        else
        {
            return;                                       // stands still
        }

        // animation: exactly the same idea as the player's (running uses the running frames)
        const bool run_frames = moving && npc.running;
        if(moving)
        {
            if(++npc.anim_timer >= (run_frames ? 5 : 8)) { npc.anim_timer = 0; npc.step = (npc.step + 1) % 4; }
        }
        else
        {
            npc.step = 0;
            npc.anim_timer = 0;
        }

        const int wanted = first_frame(npc.dir, run_frames) + walk_cycle[npc.step];
        if(wanted != npc.shown)
        {
            npc.shown = wanted;
            npc.sprite.set_tiles(npc.def->sprite_item->tiles_item(), wanted);
        }
    }

    void face_player(Npc& npc, int player_x, int player_y)
    {
        const int dx = player_x - npc.x.integer();
        const int dy = player_y - npc.y.integer();
        if(abs_int(dx) > abs_int(dy)) { npc.dir = dx < 0 ? LEFT : RIGHT; }
        else                          { npc.dir = dy < 0 ? UP : DOWN; }

        npc.step = 0;
        npc.shown = first_frame(npc.dir, false);      // standing frame, looking at the player
        npc.sprite.set_tiles(npc.def->sprite_item->tiles_item(), npc.shown);
    }

    // ================================================================================
    // THE DIALOGUE BOX
    // ================================================================================

    struct Conversation
    {
        Npc* npc = nullptr;                          // who we are talking to; nullptr = nobody
        int node = 0;                                // which DialogueNode of theirs is on screen
        int choice = 0;                              // which answer is highlighted (when the node asks something)
        bn::vector<bn::sprite_ptr, 48> text_sprites; // the name and the line
        bn::vector<bn::sprite_ptr, 32> choice_sprites;
        bn::vector<bn::sprite_ptr, 4> box;           // the black box behind the text

        bool active() const { return npc != nullptr; }
    };

    void end_conversation(Conversation& c)
    {
        c.npc = nullptr;
        c.text_sprites.clear();
        c.choice_sprites.clear();
        c.box.clear();
    }

    // the answers sit side by side on the bottom row; the highlighted one gets a "> " in front
    void draw_choices(Conversation& c, bn::sprite_text_generator& text)
    {
        const DialogueNode& node = c.npc->def->nodes[c.node];
        const int n = node.choice_count;
        const int spacing = n <= 2 ? 120 : 80;

        c.choice_sprites.clear();
        for(int i = 0; i < n; ++i)
        {
            bn::string<24> label(i == c.choice ? "> " : "  ");
            label.append(node.choices[i].text);
            text.generate((2 * i - (n - 1)) * spacing / 2, 72, label, c.choice_sprites);
        }
    }

    // put a node on screen (and do whatever the node says should happen)
    void show_node(Conversation& c, bn::sprite_text_generator& text)
    {
        Npc& npc = *c.npc;
        const DialogueNode& node = npc.def->nodes[c.node];

        if(node.action == ACT_FOLLOW)
        {
            npc.following = true;
        }
        else if(node.action == ACT_STOP_FOLLOW)
        {
            npc.following = false;
            npc.running = false;
            npc.home_x = npc.x.integer();             // from now on they wander around the spot they were told to wait at
            npc.home_y = npc.y.integer();
            npc.state = NPC_WAIT;
            npc.timer = 60;
        }
        if(node.set_start >= 0) { npc.start_node = node.set_start; }

        c.text_sprites.clear();
        c.choice_sprites.clear();
        text.generate(-90, 40, npc.def->name, c.text_sprites);
        text.generate(0, 56, node.text, c.text_sprites);

        c.choice = 0;
        if(node.choice_count > 0) { draw_choices(c, text); }
    }

    void start_conversation(Conversation& c, Npc& npc, int player_x, int player_y, bn::sprite_text_generator& text)
    {
        c.npc = &npc;
        c.node = npc.start_node;
        face_player(npc, player_x, player_y);

        for(int i = 0; i < 4; ++i)                   // a black box behind the text (reuses your transition sprite)
        {
            bn::sprite_ptr b = bn::sprite_items::black_block.create_sprite(bn::fixed(-96 + i * 64), 64);
            b.set_bg_priority(0);
            c.box.push_back(b);
        }
        show_node(c, text);
    }

    // called every frame while a conversation is open
    void update_conversation(Conversation& c, bn::sprite_text_generator& text)
    {
        const DialogueNode& node = c.npc->def->nodes[c.node];
        bool advance = false;
        int next = node.next;

        if(node.choice_count > 0)
        {
            if(bn::keypad::left_pressed() && c.choice > 0)
            {
                --c.choice;
                draw_choices(c, text);
            }
            else if(bn::keypad::right_pressed() && c.choice < node.choice_count - 1)
            {
                ++c.choice;
                draw_choices(c, text);
            }

            if(bn::keypad::a_pressed())
            {
                next = node.choices[c.choice].next;
                advance = true;
            }
        }
        else if(bn::keypad::a_pressed())
        {
            advance = true;
        }

        if(advance)
        {
            if(next == DLG_END)
            {
                end_conversation(c);
            }
            else
            {
                c.node = next;
                show_node(c, text);
            }
        }
    }

    // ================================================================================
    // SCREENS
    // ================================================================================

    void title_screen(bn::sprite_text_generator& text)
    {
        bn::regular_bg_ptr mountains = bn::regular_bg_items::layer_2_mountains.create_bg(0, 0);
        bn::regular_bg_ptr hills     = bn::regular_bg_items::layer_3_hills.create_bg(0, 0);
        bn::regular_bg_ptr forest    = bn::regular_bg_items::layer_4_forest.create_bg(0, 0);
        mountains.set_priority(3);   // Layers, 3 = furthest back
        hills.set_priority(2);
        forest.set_priority(1);
        text.set_bg_priority(0);

        bn::fixed camera = 0;
        bn::fixed step = 0.25;
        const bn::fixed mountains_speed = 0.25;
        const bn::fixed hills_speed = 0.5;

        bn::vector<bn::sprite_ptr, 32> title;
        bn::vector<bn::sprite_ptr, 32> prompt;
        bn::vector<bn::sprite_ptr, 32> text_sprites;

        text.set_center_alignment();
        text.generate(0, -20, "FLUIDIA", title);
        enlarge(title, -20, 2);

        text.generate(0, 40, "PRESS START", prompt);
        text.generate(-65,70, "version " GAME_VER, text_sprites);
        text.generate(70,70, "By Waffles Fox", text_sprites);

        bool visible = true;
        int frame = 0;

        // if the music chunk finishes, another will play (wow)
        int chunk = 0;
        bn::sound_handle music = music_chunk(chunk).play();

        while(!bn::keypad::start_pressed())
        {
            if(++frame == 30)
            {
                frame = 0;
                visible = !visible;
                for(bn::sprite_ptr& s : prompt) { s.set_visible(visible); }
            }

            camera += step;
            if(camera > 40 || camera < -40) { step = -step; }   // camera control woooo

            mountains.set_x(-camera * mountains_speed);
            hills.set_x(-camera * hills_speed);
            forest.set_x(-camera);
            if(!music.active())   // this chunk finished, so start the next one
            {
                chunk = (chunk + 1) % music_chunks;
                music = music_chunk(chunk).play();
            }

            bn::core::update();
        }
        music.stop();
        // everything gets destroyed in this bounds
    }

    void game_screen(bn::sprite_text_generator& text)
    {
        int current_map = MAP_HOUSE;
        bn::optional<bn::regular_bg_ptr> ground_bg = make_ground(current_map);
        bn::optional<bn::regular_bg_ptr> top_bg = make_top(current_map);
        ground_bg->set_priority(2);                       // ground: behind the player (backshot the tree)
        if(top_bg) { top_bg->set_priority(0); }           // roofs and canopies: in front of the player

        // the player's position in WORLD pixels (the middle of the first world)
        bn::fixed x = 7 * T;
        bn::fixed y = 11 * T;

        bn::vector<bn::sprite_ptr, 32> text_sprites;
        bn::vector<bn::sprite_ptr, 32> pause_text_sprites;
        text.set_center_alignment();
        text.generate(-65,70, map_name(current_map), text_sprites);

        int chunk = 0;
        bn::sound_handle music = gameMusic_chunk(chunk).play();
        // bn::sprite_ptr player = bn::sprite_items::player.create_sprite(0, -40);
        // bn::fixed vy = 0;
        // const bn::fixed gravity = 0.15;
        // const bn::fixed floor_y = 64;
        int dir = DOWN;
        int step = 0;
        int timer = 0;
        int shown = first_frame(dir, false);
        bn::sprite_ptr waffle = bn::sprite_items::waffle.create_sprite(0, 0, shown);
        waffle.set_bg_priority(1); // frontshot the tree (between the ground and the roofs)
        bool paused = true;

        const int feet = 12;   // check the ground at the players feet, not the middle of the sprite
        
        init_npc_saves();
        bn::vector<Npc, 8> npcs;              // the NPCs of the CURRENT map only (rebuilt on every map change)
        spawn_npcs(npcs, current_map, x.integer(), y.integer());

        Conversation talk;                    // the dialogue box: who we talk to, which node, which answer is picked

        // screen transition state
        bn::vector<bn::sprite_ptr, 12> blocks;   // the black block
        int trans_phase = 0;                     // 0 = none, 1 = block closing over the screen, 2 = block moving away
        int trans_warp = 0;                      // which warp of the current map we are going through
        int trans_dir = DOWN;                    // the way the player was facing when it started
        bn::fixed trans_pos = 0;                 // 0 = the block covers the whole screen
        // bool snepTalked = false;
        // DEBUGGING THE TEXT BOX
        // bn::vector<bn::sprite_ptr, 8> test_sprites;
        // text.generate(0, -40, "TEST", test_sprites);
        while(true)
        {
            if(bn::keypad::start_pressed())
            {
                paused = !paused;

                if(paused)
                {
                    text.generate(0, 0, "PAUSED", pause_text_sprites);
                }
                else
                {
                    pause_text_sprites.clear();
                }
            }
            if (!paused)
            {
                bn::fixed dx = 0;
                bn::fixed dy = 0;
                if(bn::keypad::left_held())  { dx -= 1; }
                if(bn::keypad::right_held()) { dx += 1; }
                if(bn::keypad::up_held())    { dy -= 1; }
                if(bn::keypad::down_held())  { dy += 1; }
                if(trans_phase != 0 || talk.active()) { dx = 0; dy = 0; }   // the player stands still while the screen wipes and talking

                const bool moving = dx != 0 || dy != 0;
                const bool running = moving && bn::keypad::b_held();
                const bn::fixed speed = running ? 1.5 : 1;

                if(moving)
                {
                    if(dx < 0)      { dir = LEFT;  }
                    else if(dx > 0) { dir = RIGHT; }
                    else if(dy < 0) { dir = UP;    }
                    else            { dir = DOWN;  }
                    
                    // NPC collissions
                    bn::fixed next_x = x + dx * speed;
                    if(map_walkable(current_map, next_x.integer(), (y + feet).integer()) &&
                       !blocked_by_npc(npcs, x.integer(), y.integer(), next_x.integer(), y.integer())) { x = next_x; }
                    bn::fixed next_y = y + dy * speed;
                    if(map_walkable(current_map, x.integer(), (next_y + feet).integer()) &&
                       !blocked_by_npc(npcs, x.integer(), y.integer(), x.integer(), next_y.integer())) { y = next_y; }

                    if(++timer >= (running ? 5 : 8))   // lower number = faster animation
                    {
                        timer = 0;
                        step = (step + 1) % 4;
                    }
                }
                else
                {
                    step = 0;
                    timer = 0;
                }

                const int wanted = first_frame(dir, running) + walk_cycle[step];
                if(wanted != shown)    // only touch the graphics when the frame actually changes
                {
                    shown = wanted;
                    waffle.set_tiles(bn::sprite_items::waffle.tiles_item(), shown);
                }
                
                // ---- NPCs: wander or follow (they freeze while you talk) ----
                if(!talk.active())
                {
                    for(Npc& n : npcs)
                    {
                        update_npc(n, npcs, x.integer(), y.integer());
                    }
                }

                // ---- talking ----
                if(talk.active())
                {
                    update_conversation(talk, text);
                }
                else if(trans_phase == 0 && bn::keypad::a_pressed())
                {
                    Npc* target = npc_in_reach(npcs, x.integer(), y.integer());   // nearest NPC within reach
                    if(target)
                    {
                        start_conversation(talk, *target, x.integer(), y.integer(), text);
                    }
                }

                // ---- warps: did the player's feet step into one? ----
                int warp_count = 0;
                const Warp* warps = map_warps(current_map, warp_count);
                if(trans_phase == 0)
                {
                    for(int i = 0; i < warp_count; ++i)
                    {
                        const Warp& w = warps[i];
                        const int fx = x.integer();
                        const int fy = (y + feet).integer();

                        if(fx >= w.x && fx < w.x + w.w && fy >= w.y && fy < w.y + w.h)
                        {
                            trans_phase = 1;                          // start closing the screen
                            trans_warp = i;
                            trans_dir = dir;
                            trans_pos = transition_distance(dir);
                            make_blocks(blocks);
                            break;
                        }
                    }
                }

                // ---- the transition itself ----
                if(trans_phase != 0)
                {
                    trans_pos -= TRANSITION_SPEED;

                    if(trans_phase == 1 && trans_pos <= 0)
                    {
                        trans_pos = 0;                                // the screen is fully covered: swap the maps now
                        const Warp& w = warps[trans_warp];
                        current_map = w.dest_map;
                        x = w.dest_x;
                        y = w.dest_y;

                        ground_bg.reset();                            // free the old map first
                        top_bg.reset();
                        ground_bg = make_ground(current_map);
                        top_bg = make_top(current_map);
                        ground_bg->set_priority(2);
                        if(top_bg) { top_bg->set_priority(0); }

                        end_conversation(talk);                       // (safety) never keep pointing at an NPC that is about to vanish
                        spawn_npcs(npcs, current_map, x.integer(), y.integer());   // old NPCs are stored + freed, new ones created, followers tag along

                        text_sprites.clear();                         // update the name in the corner
                        text.generate(-65,70, map_name(current_map), text_sprites);

                        trans_phase = 2;                              // now slide the block away
                    }
                    else if(trans_phase == 2 && trans_pos <= -transition_distance(trans_dir))
                    {
                        blocks.clear();                               // the block has left the screen
                        trans_phase = 0;
                    }

                    if(trans_phase != 0)
                    {
                        move_blocks(blocks, trans_dir, trans_pos);
                    }
                }

                // ---- camera: follows the player, stops at the edges of the current map ----
                const int map_w = map_width(current_map);
                const int map_h = map_height(current_map);
                const bn::fixed cam_x = clamp_fixed(x, 120, map_w - 120);
                const bn::fixed cam_y = clamp_fixed(y, 80, map_h - 80);

                const bn::fixed bg_x = bn::fixed(map_w / 2) - cam_x;
                const bn::fixed bg_y = bn::fixed(map_h / 2) - cam_y;
                ground_bg->set_position(bg_x, bg_y);
                if(top_bg) { top_bg->set_position(bg_x, bg_y); }

                // Draw CHaracters
                waffle.set_position(x - cam_x, y - cam_y);
                for(Npc& n : npcs)
                {
                    n.sprite.set_visible(true);
                    n.sprite.set_position(n.x - cam_x, n.y - cam_y);
                    n.sprite.set_z_order(-n.y.integer());         // lower on the map = drawn in front
                }

                // depth sorting: whoever is lower on the map is drawn in front
                waffle.set_z_order(-y.integer());
            }

            if(!music.active())   // this chunk finished, so start the next one
            {
                chunk = (chunk + 1) % music_chunks;
                music = gameMusic_chunk(chunk).play();
            }
            bn::core::update();
        }
        music.stop();
    }
}

int main()
{
    bn::core::init();

    // while(true){ debugging crap, because GBA music is weird
    //     if(bn::keypad::l_pressed()) { bn::sound_items::title_20.play(); }
    //     if(bn::keypad::b_pressed()) { bn::sound_items::title_25.play(); }
    //     if(bn::keypad::r_pressed()) { bn::sound_items::title.play(); }
    //     bn::core::update();
    // }
    // bn::sound_items::beep.play();
    bn::bg_palettes::set_transparent_color(bn::color(10, 20, 31));  // the main bg

    bn::sprite_text_generator text(common::variable_8x16_sprite_font);

    title_screen(text);
    game_screen(text);
}
