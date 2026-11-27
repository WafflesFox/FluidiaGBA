#ifndef GAME_NPC_AI_H
#define GAME_NPC_AI_H

// ================================================================================================
// NPC behaviour: collisions, wandering, following, talking-to-the-player
//
// Part of main.cpp. Include it ONLY from main.cpp, after main.cpp's own #includes and in the order
// listed there: it uses everything main.cpp has declared before it.
// ================================================================================================

namespace
{
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

    // ---- WHO COMES ALONG TO A BATTLE ----
    // If NPCs are following you when a battle starts, only ONE of them takes part:
    //   - one with fighting data joins your side (2 against 1)
    //   - one without it can't fight, so it just gives you a buff from its traits (see buff_from_traits)
    // With several followers, the first one that CAN fight is taken; if none can, the first one in the npc_defs table.
    // The NPC you are fighting is never picked (it can't be on both sides). Returns an NPC id, or -1 for nobody.
    // (Call it BEFORE park_npcs: that empties the list.)
    int pick_companion(const bn::ivector<Npc>& npcs, int foe_id)
    {
        int best = -1;
        for(const Npc& n : npcs)
        {
            if(!n.following || n.id == foe_id) { continue; }
            if(best < 0 || (!npc_defs[best].fight && n.def->fight)) { best = n.id; }
        }
        return best;
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
}

#endif
