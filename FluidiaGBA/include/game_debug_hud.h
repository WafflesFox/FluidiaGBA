#ifndef GAME_DEBUG_HUD_H
#define GAME_DEBUG_HUD_H

// ================================================================================================
// Debug HUD: live memory counters drawn on screen while DEBUG MODE is on (SELECT+L+R toggles it)
//
// Part of main.cpp. Include it ONLY from main.cpp (after game_common.h, before game_play.h).
//
// What it shows (top centre of the screen, works in the overworld AND during battles):
//
//     IW 24K/32K      IWRAM used (static + stack) out of 32K
//     STK 812/1290    stack bytes right now / biggest value seen since debug mode was switched on
//     EW 140K F98K    EWRAM used (static + heap) / F = heap still free for malloc, vector, etc.
//     SPR 87/128      hardware sprites in use (not counting the HUD's own text) / the 128 limit
//
// The HUD only redraws every few frames (text sprites are not free), and it politely backs off when
// sprites are nearly gone so that it can never be the thing that crashes the game:
//   plenty of sprites left  -> all four lines
//   getting low             -> only the SPR line
//   almost none left        -> nothing at all
// ================================================================================================

#include "bn_memory.h"
#include "bn_sprites.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_string.h"
#include "bn_vector.h"

namespace
{
    class DebugHud
    {
    public:
        // Call once per frame, right before bn::core::update(). `enabled` is your debug_mode flag.
        void update(bn::sprite_text_generator& text, bool enabled)
        {
            if(!enabled)
            {
                hide();
                _peak_stack = 0;
                return;
            }

            const int stack = bn::memory::used_stack_iwram();
            if(stack > _peak_stack) { _peak_stack = stack; }

            if(_timer > 0) { --_timer; return; }
            _timer = REFRESH_FRAMES - 1;
            draw(text, stack);
        }

        // Frees the HUD's sprites (used before VRAM-hungry screens). It comes back on the next update().
        void hide()
        {
            _sprites.clear();
            _timer = 0;
        }

    private:
        static constexpr int REFRESH_FRAMES = 15;    // redraw 4 times a second
        static constexpr int HUD_Y          = -72;   // y of the first line (top of the screen)
        static constexpr int LINE_H         = 13;
        static constexpr int FULL_SPRITES   = 49;    // worst case sprites for all four lines
        static constexpr int ONE_SPRITES    = 11;    // worst case sprites for the SPR line alone
        static constexpr int RESERVE        = 16;    // sprites we always leave for the game itself
        static constexpr int IWRAM_TOTAL    = 32 * 1024;

        static int kb(int bytes) { return (bytes + 512) / 1024; }

        static void add_int(bn::istring& s, int value)
        {
            s.append(bn::to_string<12>(value));
        }

        void draw(bn::sprite_text_generator& text, int stack)
        {
            _sprites.clear();    // free our own sprites first so the SPR numbers below don't count them

            const int sprites_used = bn::sprites::used_items_count();
            const int sprites_free = bn::sprites::available_items_count();

            const bool full = sprites_free >= FULL_SPRITES + RESERVE;
            const bool one  = sprites_free >= ONE_SPRITES + RESERVE;
            if(!full && !one) { return; }

            bn::string<24> spr("SPR ");
            add_int(spr, sprites_used);
            spr.append("/");
            add_int(spr, sprites_used + sprites_free);

            text.set_center_alignment();
            int row = 0;

            if(full)
            {
                bn::string<24> iw("IW ");
                add_int(iw, kb(bn::memory::used_static_iwram() + stack));
                iw.append("K/");
                add_int(iw, IWRAM_TOTAL / 1024);
                iw.append("K");

                bn::string<24> stk("STK ");
                add_int(stk, stack);
                stk.append("/");
                add_int(stk, _peak_stack);

                bn::string<24> ew("EW ");
                add_int(ew, kb(bn::memory::used_static_ewram() + bn::memory::used_alloc_ewram()));
                ew.append("K F");
                add_int(ew, kb(bn::memory::available_alloc_ewram()));
                ew.append("K");

                text.generate(0, HUD_Y + (row++) * LINE_H, iw, _sprites);
                text.generate(0, HUD_Y + (row++) * LINE_H, stk, _sprites);
                text.generate(0, HUD_Y + (row++) * LINE_H, ew, _sprites);
            }

            text.generate(0, HUD_Y + row * LINE_H, spr, _sprites);

            for(bn::sprite_ptr& sp : _sprites)
            {
                sp.set_bg_priority(0);
                sp.set_z_order(-32000);    // in front of everything, the battle debug menu included
            }
        }

        bn::vector<bn::sprite_ptr, 64> _sprites;
        int _timer = 0;
        int _peak_stack = 0;
    };
}

#endif
