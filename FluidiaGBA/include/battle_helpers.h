#ifndef BATTLE_HELPERS_H
#define BATTLE_HELPERS_H

// Small helpers: format_int() and reading the player's buttons (normal + link-battle lockstep).

#include "bn_keypad.h"
#include "battle_logic.h"
#include "net_link.h"

namespace battle
{
    // ============================================================================================
    // SMALL HELPERS
    // ============================================================================================

    inline void format_int(int value, char* out)
    {
        char tmp[12];
        int t = 0, n = 0;
        if(value < 0) { out[n++] = '-'; value = -value; }
        if(value == 0) { tmp[t++] = '0'; }
        while(value > 0) { tmp[t++] = char('0' + value % 10); value /= 10; }
        while(t > 0) { out[n++] = tmp[--t]; }
        out[n] = 0;
    }

    // the player's buttons -> what the rules need
    inline Input read_player_input()
    {
        Input in;
        if(bn::keypad::left_held())  { in.move -= 1; }
        if(bn::keypad::right_held()) { in.move += 1; }
        in.up      = bn::keypad::up_held();
        in.down    = bn::keypad::down_held();
        in.jump    = bn::keypad::r_pressed();
        in.attack  = bn::keypad::a_pressed();
        in.special = bn::keypad::b_pressed();
        in.special_held = bn::keypad::b_held();
        in.shield  = bn::keypad::l_held();
        in.final_smash = bn::keypad::select_pressed();
        return in;
    }

    // Link battles: the RAW held buttons (the "pressed this frame" edges are derived later, identically on both consoles)
    inline unsigned read_player_held()
    {
        unsigned h = 0;
        if(bn::keypad::left_held())   { h |= net::Lockstep::H_LEFT; }
        if(bn::keypad::right_held())  { h |= net::Lockstep::H_RIGHT; }
        if(bn::keypad::up_held())     { h |= net::Lockstep::H_UP; }
        if(bn::keypad::down_held())   { h |= net::Lockstep::H_DOWN; }
        if(bn::keypad::l_held())      { h |= net::Lockstep::H_SHIELD; }
        if(bn::keypad::r_held())      { h |= net::Lockstep::H_JUMP; }
        if(bn::keypad::a_held())      { h |= net::Lockstep::H_ATTACK; }
        if(bn::keypad::b_held())      { h |= net::Lockstep::H_SPECIAL; }
        if(bn::keypad::select_held()) { h |= net::Lockstep::H_FS; }
        return h;
    }

    inline Input expand_held(unsigned h, unsigned prev)
    {
        Input in;
        if(h & net::Lockstep::H_LEFT)  { in.move -= 1; }
        if(h & net::Lockstep::H_RIGHT) { in.move += 1; }
        in.up      = (h & net::Lockstep::H_UP) != 0;
        in.down    = (h & net::Lockstep::H_DOWN) != 0;
        in.shield  = (h & net::Lockstep::H_SHIELD) != 0;
        const unsigned rising = h & ~prev;
        in.jump        = (rising & net::Lockstep::H_JUMP) != 0;
        in.attack      = (rising & net::Lockstep::H_ATTACK) != 0;
        in.special     = (rising & net::Lockstep::H_SPECIAL) != 0;
        in.special_held = (h & net::Lockstep::H_SPECIAL) != 0;
        in.final_smash = (rising & net::Lockstep::H_FS) != 0;
        return in;
    }
}

#endif
