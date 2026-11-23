#ifndef BATTLE_H
#define BATTLE_H

// ================================================================================================
// THE BATTLE SCREEN:
//
//   A = normal attacks: A (jab), A + direction on the ground (smashes), A in the air (aerials, + direction)
//   B = special attacks: B (water gun), B + left/right (aqua jet), B + up (aqua jet up), B + down (fox thief)
//   R = jump (press again in the air for a double jump)
//   LEFT / RIGHT = move  DOWN (held) = drop through a platform / fall faster
//   L (hold) = shield: a big defense boost that slowly fades the longer you hold it
//
//   The moves themselves are data in battle_moves.h. The rules are in battle_logic.h.
//
//   battle_ui.start(...)   when the screen is covered by the black blocks
//   battle_ui.update(...)  every frame while battle_ui.active()
//   battle_ui.release_blocks(blocks) when it finishes (the screen is covered again)
//
// ================================================================================================

#include "bn_color.h"
#include "bn_fixed.h"
#include "bn_keypad.h"
#include "bn_optional.h"
#include "bn_regular_bg_item.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_affine_mat_ptr.h"
#include "bn_sprite_item.h"
#include "bn_sprite_palette_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "bn_sprite_items_black_block.h"
#include "bn_sprite_items_water_shot.h"
#include "bn_sprite_items_battle_floor.h"
#include "bn_sprite_items_battle_platform.h"
#include "bn_sprite_items_hp_bar.h"
#include "bn_sprite_items_shield.h"
#include "bn_sprite_items_explosion.h"
#include "bn_sprite_items_aqua_jet.h"
#include "bn_sprite_items_swipe.h"
#include "bn_sprite_items_hit_spark.h"

#include "battle_logic.h"

namespace battle
{
    // ============================================================================================
    // DATA YOU WRITE (in main.cpp): fighters and stages
    // ============================================================================================

    // A fighter = a name, a sprite sheet (the same layout as the overworld sheet) and numbers (see FighterStats).
    struct FighterDef
    {
        const char* name;
        const bn::sprite_item* sprite;
        FighterStats stats;
    };

    // One scrolling background picture. speed_percent: 100 = moves as fast as the camera (the closest layer),
    // 25 = moves slowly (far away). The farthest layer goes first. (Backgrounds repeat sideways, so a picture
    // that tiles left-to-right looks best on long stages.)
    struct ParallaxLayer
    {
        const bn::regular_bg_item* item;
        int speed_percent;
    };

    // A stage = the shape of the ground (see StageGeom) + up to 3 background layers.
    struct StageDef
    {
        StageGeom geom;
        ParallaxLayer layers[3];
        int layer_count;
    };

    // ---- things main.cpp can react to (sound effects) ----
    enum : unsigned
    {
        EV_SHOT    = 1 << 0,    // somebody fired
        EV_HIT     = 1 << 1,    // somebody got hit
        EV_BLOCKED = 1 << 2,    // ...and it landed on a shield
        EV_JUMP    = 1 << 3,
        EV_KO      = 1 << 4,
        EV_FIGHT   = 1 << 5,    // the fight just started
        EV_SWING   = 1 << 6,    // a melee move started (jab, smash, aerial...)
        EV_DASH    = 1 << 7,    // an aqua jet took off
        EV_ABSORB  = 1 << 8,    // fox thief stole a shot
        EV_BOOM    = 1 << 9,    // the dair explosion
    };

    // ============================================================================================
    // WHICH PICTURE TO SHOW. You have no jump / attack / shield animations yet, so the walking and
    // running frames stand in for them. When you draw real ones, only this function needs to change.
    // The sheet is 4 directions x 3 frames, and the running frames come after the walking ones.
    // ============================================================================================

    constexpr int DIR_UP = 0, DIR_DOWN = 1, DIR_LEFT = 2, DIR_RIGHT = 3, SHIELD = 28, KO = 24, HURT = 25;
    //waffles specific animation
    constexpr int WAF_SIDE_JET = 30, WAF_UP_JET = 32;
    constexpr int walk_frames[4] = { 0, 1, 0, 2 };

    constexpr int waff_jab[4] = {33,34,35,36};
    constexpr int waff_jab_left_ofset = 4;

    constexpr int SPRITE_FEET = 12;                  // from the middle of the sprite down to the feet (as in the overworld)

    inline int sheet_frame(int dir, bool running)
    {
        return (running ? 12 : 0) + dir * 3;
    }

    // The pose during a move. These are all stand-ins made from the frames you already have: moves that go up show
    // the "up" frames, moves that go down show the "down" frames, and the spin turns through all four directions.
    inline int attack_frame(int anim, int facing, int side, int move_frame)
    {
        const int WAF_DIR_JET = facing > 0 ? WAF_SIDE_JET : WAF_SIDE_JET+1;
        // const int WAF_JAB_SIDE = facing > 0 ? waff_jab[move_frame] : waff_jab[move_frame]+4;
        switch(anim)
        {
            // Waffles MoveSet
            case ANIM_SLAP:
                if(move_frame < 30) { return waff_jab[(move_frame % 10) * 4 / 10] + (side == DIR_LEFT ? waff_jab_left_ofset : 0); }
                return sheet_frame(side, false);
            case ANIM_TAIL:
            case ANIM_KICK:
            case ANIM_SHOOT:    return sheet_frame(side, true) + 2;
            case ANIM_TAIL_UP:
            case ANIM_JET_UP:   return WAF_UP_JET;
            case ANIM_SLAM:
            case ANIM_JET_DOWN: return sheet_frame(DIR_DOWN, true) + 1;
            case ANIM_JET_SIDE: return WAF_DIR_JET;
            case ANIM_SPIN:
            {
                static const int order[4] = { DIR_DOWN, DIR_RIGHT, DIR_UP, DIR_LEFT };
                return sheet_frame(order[(move_frame / 3) % 4], false);
            }
            case ANIM_STANCE:   return sheet_frame(DIR_DOWN, false);
            default:            return sheet_frame(side, true);
        }
    }

    // `anim` and `move_frame` only matter while attacking (state == ST_ATTACK)
    inline int pick_frame(int state, int facing, int step, int anim = ANIM_STAND, int move_frame = 0)
    {
        const int side = facing > 0 ? DIR_RIGHT : DIR_LEFT;
        const int shield_side = facing > 0 ? SHIELD : SHIELD+1;
        const int ko_side = facing > 0 ? KO : KO+1;
        const int hurt_side = facing > 0 ? HURT : HURT +2;
        switch(state)
        {
            case ST_WALK:     return sheet_frame(side, true) + walk_frames[step];     // the running frames
            case ST_AIR:      return sheet_frame(side, true) + 1;                     // placeholder for a jump animation
            case ST_ATTACK:   return attack_frame(anim, facing, side, move_frame);            // placeholders for the attack animations
            case ST_SHIELD:   return shield_side;
            case ST_HURT:     return hurt_side;
            case ST_KO:       return ko_side;
            default:          return sheet_frame(side, false);                        // standing (and flinching)
        }
    }

    // What a shot looks like: its sprite sheet, how many pictures it has, and how many game frames each is shown.
    // A new PROJ_... kind (battle_logic.h) needs a line here.
    struct ProjArt
    {
        const bn::sprite_item* item;
        int frames;
        int ticks;
    };

    inline ProjArt projectile_art(int kind)
    {
        switch(kind)
        {
            case PROJ_EXPLOSION: return ProjArt{ &bn::sprite_items::explosion, 4, 3 };
            default:             return ProjArt{ &bn::sprite_items::water_shot, 2, 5 };   // PROJ_WATER
        }
    }

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
        in.shield  = bn::keypad::l_held();
        return in;
    }

    // ============================================================================================
    // ONE FIGHTER ON SCREEN: the character, the shield bubble, the health bar and the name
    // ============================================================================================

    struct FighterView
    {
        bn::sprite_ptr sprite;
        bn::sprite_ptr shield;
        bn::sprite_affine_mat_ptr shield_mat;
        bn::sprite_ptr hp_back;
        bn::sprite_ptr hp_fill;
        bn::sprite_affine_mat_ptr hp_mat;
        bn::vector<bn::sprite_ptr, 12> name;
        bn::vector<bn::sprite_ptr, 5> hp_text;
        bn::optional<bn::sprite_ptr> swipes[2];     // slash pictures where melee hitboxes are active
        bn::optional<bn::sprite_ptr> jet;           // the aqua jet bubble
        bn::optional<bn::sprite_ptr> held;          // the shot fox thief is holding above the head
        const bn::sprite_item* sheet = nullptr;     // the sprite sheet this character was made from
        int shown_frame = -1;
        int step = 0;
        int anim_timer = 0;
        int shown_hp = -1;
        bool on_right = false;                     // the opponent's HUD is on the right

        FighterView(const FighterDef& def, bool opponent, const char* label, bn::sprite_text_generator& text) :
            sprite(def.sprite->create_sprite(0, 0, pick_frame(ST_IDLE, opponent ? -1 : 1, 0))),   // made at its real first frame: tiles are only loaded once
            shield(bn::sprite_items::shield.create_sprite(0, 0)),
            shield_mat(bn::sprite_affine_mat_ptr::create()),
            hp_back(bn::sprite_items::hp_bar.create_sprite(0, 0, 0)),
            hp_fill(bn::sprite_items::hp_bar.create_sprite(0, 0, 1)),
            hp_mat(bn::sprite_affine_mat_ptr::create()),
            on_right(opponent)
        {
            sheet = def.sprite;
            shown_frame = pick_frame(ST_IDLE, opponent ? -1 : 1, 0);
            sprite.set_bg_priority(0);
            sprite.set_z_order(opponent ? 0 : -1);                    // the player is drawn in front

            if(opponent)                                              // tint the opponent red so you can tell two Waffles apart
            {
                bn::sprite_palette_ptr tint = bn::sprite_palette_ptr::create_new(def.sprite->palette_item());
                tint.set_fade(bn::color(31, 6, 6), 0.35);
                sprite.set_palette(tint);
            }

            shield.set_bg_priority(0);
            shield.set_z_order(-3);
            shield.set_affine_mat(shield_mat);
            shield.set_visible(false);

            hp_back.set_bg_priority(0);
            hp_back.set_z_order(-10);
            hp_fill.set_bg_priority(0);
            hp_fill.set_z_order(-11);
            hp_fill.set_affine_mat(hp_mat);

            const int bar_center_x = opponent ? 96 : -96;             // the bars are 32 pixels wide
            hp_back.set_position(bar_center_x, -58);
            hp_fill.set_position(bar_center_x, -58);

            if(opponent) { text.set_right_alignment(); text.generate(112, -72, label, name); }
            else         { text.set_left_alignment();  text.generate(-112, -72, label, name); }
            text.set_center_alignment();
        }
    };

    // ============================================================================================
    // THE BATTLE
    // ============================================================================================

    class Battle
    {
    public:
        unsigned events = 0;                       // EV_... flags of the frame that just ran

        bool active() const { return _phase != PH_OFF; }

        // +1 = the player won, -1 = the player lost (read it after active() turns false)
        int result() const { return _result; }

        // Call when the screen is fully covered. `covering_blocks` are main.cpp's black blocks: the battle takes
        // them over (and slides them away). The player always uses the first fighter; `seed` makes the fights differ.
        void start(const FighterDef& player, const FighterDef& enemy, const StageDef& stage,
                   bn::ivector<bn::sprite_ptr>& covering_blocks, bn::sprite_text_generator& text, unsigned seed)
        {
            (void)text;
            _stage = &stage;
            _player = &player;
            _enemy = &enemy;
            _result = 0;
            events = 0;

            for(bn::sprite_ptr& block : covering_blocks) { _blocks.push_back(block); }
            covering_blocks.clear();
            _wipe = 0;
            move_blocks();

            _fight.start(stage.geom, player.stats, enemy.stats, seed);

            // The stage itself is built on the NEXT frame (see PH_SETUP): the overworld was just thrown away, and
            // video memory is much happier if the new pictures are loaded a frame later instead of in the same one. YAY :3
            _phase = PH_SETUP;
        }

        // one frame. `text` is the game's text generator.
        void update(bn::sprite_text_generator& text)
        {
            events = 0;

            switch(_phase)
            {
                case PH_SETUP:
                    build_scene(text);
                    _phase = PH_OPENING;
                    break;

                case PH_OPENING:
                    _wipe -= WIPE_SPEED;                                // the blocks slide off to the left
                    if(_wipe <= -WIPE_DISTANCE)
                    {
                        _blocks.clear();
                        _phase = PH_READY;
                        _timer = 60;
                        show_banner(text, "READY?");
                    }
                    else { move_blocks(); }
                    break;

                case PH_READY:
                    if(--_timer <= 0)
                    {
                        show_banner(text, "GO!");
                        _banner_timer = 45;
                        _phase = PH_FIGHT;
                        events |= EV_FIGHT;
                    }
                    break;

                case PH_FIGHT:
                {
                    const Input in[2] = { read_player_input(), _fight.think(1) };
                    _fight.step(in);
                    collect_events();

                    if(_banner_timer > 0 && --_banner_timer == 0) { _banner.clear(); }

                    if(_fight.winner >= 0)
                    {
                        _result = _fight.winner == 0 ? 1 : -1;
                        _phase = PH_RESULT;
                        _timer = 150;
                        show_banner(text, _result > 0 ? "GAME!" : "GAY!");
                    }
                    break;
                }

                case PH_RESULT:
                {
                    const Input in[2] = {};                             // nobody can act any more, but things still settle
                    _fight.step(in);
                    if(--_timer <= 0)
                    {
                        _banner.clear();
                        make_blocks();
                        _wipe = WIPE_DISTANCE;                          // the blocks come back from the right
                        move_blocks();
                        _phase = PH_CLOSING;
                    }
                    break;
                }

                case PH_CLOSING:
                    _wipe -= WIPE_SPEED;
                    if(_wipe <= 0)
                    {
                        _wipe = 0;
                        move_blocks();
                        finish();                                       // the screen is covered: throw the stage away
                        return;
                    }
                    move_blocks();
                    break;

                default:
                    return;
            }

            draw(text);
        }

        // When active() turned false the screen is still covered. Take the blocks back to slide them away.
        void release_blocks(bn::ivector<bn::sprite_ptr>& out)
        {
            for(bn::sprite_ptr& block : _blocks) { out.push_back(block); }
            _blocks.clear();
        }

    private:
        enum Phase { PH_OFF, PH_SETUP, PH_OPENING, PH_READY, PH_FIGHT, PH_RESULT, PH_CLOSING };

        static constexpr int FLOOR_TILES = 5;           // 5 x 64 pixels: always enough to cover the screen while scrolling
        static constexpr int WIPE_SPEED = 10;           // same as the overworld transition
        static constexpr int WIPE_DISTANCE = 248;

        struct Piece
        {
            bn::sprite_ptr sprite;
            int wx;                                     // where it is on the stage (its middle)
            int wy;
        };

        // ---------------------------------------------------------------------------- building the stage

        void build_scene(bn::sprite_text_generator& text)
        {
            const StageDef& stage = *_stage;

            for(int i = 0; i < stage.layer_count; ++i)
            {
                _bgs[i] = stage.layers[i].item->create_bg(0, 0);
                _bgs[i]->set_priority(3 - i);                         // 3 = the farthest back
            }

            for(int i = 0; i < FLOOR_TILES; ++i)
            {
                bn::sprite_ptr tile = bn::sprite_items::battle_floor.create_sprite(0, 0);
                tile.set_bg_priority(0);
                tile.set_z_order(10);
                _floor.push_back(tile);
            }

            for(int k = 0; k < stage.geom.platform_count; ++k)
            {
                const Platform& pl = stage.geom.platforms[k];
                for(int j = 0; j < pl.w / 32 && !_pieces.full(); ++j)  // platforms are built from 32-pixel pieces
                {
                    bn::sprite_ptr piece = bn::sprite_items::battle_platform.create_sprite(0, 0);
                    piece.set_bg_priority(0);
                    piece.set_z_order(10);
                    _pieces.push_back(Piece{ piece, pl.x + 16 + j * 32, pl.y + 4 });
                }
            }

            _views[0] = FighterView(*_player, false, "YOU", text);
            _views[1] = FighterView(*_enemy, true, _enemy->name, text);

            _cam_x = camera_target();
            draw(text);
        }

        // ---------------------------------------------------------------------------- the wipe

        void make_blocks()
        {
            for(int row = 0; row < 3; ++row)
            {
                for(int col = 0; col < 4; ++col)
                {
                    bn::sprite_ptr block = bn::sprite_items::black_block.create_sprite(
                            bn::fixed(-96 + col * 64), bn::fixed(-64 + row * 64));
                    block.set_bg_priority(0);
                    block.set_z_order(-32000);
                    _blocks.push_back(block);
                }
            }
        }

        void move_blocks()
        {
            for(int i = 0; i < _blocks.size(); ++i)
            {
                _blocks[i].set_position(bn::fixed(-96 + (i % 4) * 64) + _wipe, bn::fixed(-64 + (i / 4) * 64));
            }
        }

        // ---------------------------------------------------------------------------- text banner

        void show_banner(bn::sprite_text_generator& text, const char* line)
        {
            _banner.clear();
            text.set_center_alignment();
            text.generate(0, -10, line, _banner);
            for(bn::sprite_ptr& s : _banner) { s.set_bg_priority(0); s.set_z_order(-20); }
        }

        void collect_events()
        {
            const Events& e = _fight.ev;
            if(e.shot)    { events |= EV_SHOT; }
            if(e.swing)   { events |= EV_SWING; }
            if(e.dash)    { events |= EV_DASH; }
            if(e.absorb)  { events |= EV_ABSORB; }
            if(e.boom)    { events |= EV_BOOM; }
            if(e.hit)     { events |= EV_HIT; }
            if(e.blocked) { events |= EV_BLOCKED; }
            if(e.jump)    { events |= EV_JUMP; }
            if(e.ko)      { events |= EV_KO; }
        }

        // throw everything away (the black blocks stay: main.cpp takes them with release_blocks)
        void finish()
        {
            _views[0].reset();
            _views[1].reset();
            for(bn::optional<bn::sprite_ptr>& p : _proj) { p.reset(); }
            for(bn::optional<bn::sprite_ptr>& e : _fx) { e.reset(); }
            _floor.clear();
            _pieces.clear();
            _banner.clear();
            for(bn::optional<bn::regular_bg_ptr>& bg : _bgs) { bg.reset(); }
            _phase = PH_OFF;
        }

        // ---------------------------------------------------------------------------- drawing

        // the camera sits between the two fighters, and stops at the ends of the stage
        bn::fixed camera_target() const
        {
            const int length = _stage->geom.length;
            if(length <= 240) { return length / 2; }

            int middle = (_fight.f[0].x16 + _fight.f[1].x16) / (2 * FP);
            if(middle < 120) { middle = 120; }
            if(middle > length - 120) { middle = length - 120; }
            return middle;
        }

        void draw(bn::sprite_text_generator& text)
        {
            _cam_x += (camera_target() - _cam_x) / 6;                   // a smooth follow
            const bn::fixed cam = _cam_x;

            // parallax: far layers move slowly, near layers keep up with the camera
            for(int i = 0; i < _stage->layer_count; ++i)
            {
                _bgs[i]->set_x(-(cam - 120) * _stage->layers[i].speed_percent / 100);
            }

            // the floor: five tiles that hop along so there is always floor under the screen
            const int left_edge = cam.integer() - 120;
            const int first_tile = (left_edge >= 0 ? left_edge / 64 : -((-left_edge + 63) / 64)) * 64;
            for(int i = 0; i < _floor.size(); ++i)
            {
                _floor[i].set_position(bn::fixed(first_tile + i * 64 + 32) - cam, _stage->geom.ground_y + 16 - 80);
            }

            for(Piece& piece : _pieces)
            {
                piece.sprite.set_position(bn::fixed(piece.wx) - cam, piece.wy - 80);
            }

            for(int i = 0; i < 2; ++i)
            {
                if(_views[i]) { draw_fighter(i, *_views[i], _fight.f[i], cam, text); }
            }

            for(int k = 0; k < MAX_PROJECTILES; ++k)
            {
                const Projectile& p = _fight.proj[k];
                if(!p.active) { _proj[k].reset(); continue; }

                const ProjArt art = projectile_art(p.shot.kind);
                if(!_proj[k])
                {
                    _proj[k] = art.item->create_sprite(0, 0, 0);
                    _proj[k]->set_bg_priority(0);
                    _proj[k]->set_z_order(-2);
                }
                _proj[k]->set_position(bn::fixed(p.x16 / FP) - cam, (p.y16 / FP) - 80);
                _proj[k]->set_horizontal_flip(p.vx < 0);
                _proj[k]->set_tiles(art.item->tiles_item(), (p.age / art.ticks) % art.frames);
            }

            // hit sparks (the sheet has 3 pictures; they play once over the effect's life)
            for(int k = 0; k < MAX_EFFECTS; ++k)
            {
                const Effect& e = _fight.fx[k];
                if(!e.active) { _fx[k].reset(); continue; }

                const int frame = e.age * 3 / e.life;
                if(!_fx[k])
                {
                    _fx[k] = bn::sprite_items::hit_spark.create_sprite(0, 0, frame);
                    _fx[k]->set_bg_priority(0);
                    _fx[k]->set_z_order(-5);
                }
                _fx[k]->set_position(bn::fixed(e.x16 / FP) - cam, (e.y16 / FP) - 80);
                _fx[k]->set_tiles(bn::sprite_items::hit_spark.tiles_item(), frame);
            }
        }

        void draw_fighter(int index, FighterView& v, const Fighter& f, bn::fixed cam, bn::sprite_text_generator& text)
        {
            const FighterStats& s = *f.stats;
            const int fx = f.x16 / FP;
            const int fy = f.y16 / FP;

            v.sprite.set_position(bn::fixed(fx) - cam, fy - SPRITE_FEET - 80);

            // animation: the walk cycle advances while walking
            if(f.state == ST_WALK)
            {
                if(++v.anim_timer >= 5) { v.anim_timer = 0; v.step = (v.step + 1) % 4; }
            }
            else { v.step = 0; v.anim_timer = 0; }

            const int frame = pick_frame(f.state, f.facing, v.step, f.move ? f.move->anim : ANIM_STAND, f.move_frame);
            if(frame != v.shown_frame)                                  // only touch the graphics when the frame changes
            {
                v.shown_frame = frame;
                v.sprite.set_tiles(v.sheet->tiles_item(), frame);
            }
            v.sprite.set_visible(!(f.state == ST_HURT && (f.timer / 2) % 2 == 0));   // blink while flinching

            // the shield bubble shrinks as its energy (= its defense boost) runs down
            // (fox thief borrows the same bubble while it is ready to steal a shot)
            const Move* move = f.state == ST_ATTACK ? f.move : nullptr;
            const bool stealing = move && move->absorb_end > move->absorb_start &&
                                  f.move_frame >= move->absorb_start && f.move_frame < move->absorb_end;
            if(f.state == ST_SHIELD)
            {
                const bn::fixed scale = bn::fixed(0.45) + bn::fixed(0.55) * f.shield / SHIELD_MAX;
                v.shield_mat.set_scale(scale);
                v.shield.set_position(bn::fixed(fx) - cam, fy - s.body_h / 2 - 80);
                v.shield.set_visible(true);
            }
            else if(stealing)
            {
                v.shield_mat.set_scale(bn::fixed(0.9));
                v.shield.set_position(bn::fixed(fx) - cam, fy - s.body_h / 2 - 80);
                v.shield.set_visible(true);
            }
            else { v.shield.set_visible(false); }

            draw_move_effects(index, v, f, move, cam);

            // the health bar: the green part is squeezed from the right
            if(f.hp != v.shown_hp)
            {
                v.shown_hp = f.hp;

                bn::fixed ratio = bn::fixed(f.hp) / s.max_hp;
                if(ratio < bn::fixed(0.02)) { ratio = bn::fixed(0.02); }
                v.hp_mat.set_horizontal_scale(ratio);
                const int left = v.on_right ? 80 : -112;
                v.hp_fill.set_x(bn::fixed(left) + bn::fixed(16) * ratio);
                v.hp_fill.set_visible(f.hp > 0);

                v.hp_text.clear();
                char number[8];
                format_int(f.hp, number);
                if(v.on_right) { text.set_right_alignment(); text.generate(72, -58, number, v.hp_text); }
                else           { text.set_left_alignment();  text.generate(-72, -58, number, v.hp_text); }
                text.set_center_alignment();
            }
        }

        // The extra pictures around a move: the aqua jet bubble, the swipes where melee hitboxes are, and the shot
        // fox thief is holding. They are made when needed and thrown away when not.
        void draw_move_effects(int index, FighterView& v, const Fighter& f, const Move* move, bn::fixed cam)
        {
            const FighterStats& s = *f.stats;
            const int fx = f.x16 / FP;
            const int fy = f.y16 / FP;

            // the aqua jet: a bubble behind the character while it flies (the sheet has side / up / down pictures)
            if(move && f.driving)
            {
                const int base = move->drive.vy < 0 ? 2 : (move->drive.vy > 0 ? 4 : 0);
                const int frame = base + (f.move_frame / 2) % 2;
                if(!v.jet)
                {
                    v.jet = bn::sprite_items::aqua_jet.create_sprite(0, 0, frame);
                    v.jet->set_bg_priority(0);
                    v.jet->set_z_order(1);
                }
                v.jet->set_tiles(bn::sprite_items::aqua_jet.tiles_item(), frame);
                v.jet->set_position(bn::fixed(fx) - cam, fy - s.body_h / 2 - 80);
                v.jet->set_horizontal_flip(f.facing < 0);
            }
            else { v.jet.reset(); }

            // swipes: until you draw real attack animations, a little slash shows where each melee hitbox is
            int used = 0;
            if(move && move->drive.end <= move->drive.start && move->anim != ANIM_SLAP)
            {
                for(int k = 0; k < move->hit_count && used < 2; ++k)
                {
                    Rect box;
                    if(!_fight.hitbox_now(index, k, box)) { continue; }

                    const Hitbox& h = move->hits[k];
                    const int length = h.end > h.start ? h.end - h.start : 1;
                    const int frame = (f.move_frame - h.start) * 3 / length;            // 0..2 over the hitbox's life
                    bn::optional<bn::sprite_ptr>& swipe = v.swipes[used];
                    if(!swipe)
                    {
                        swipe = bn::sprite_items::swipe.create_sprite(0, 0, frame);
                        swipe->set_bg_priority(0);
                        swipe->set_z_order(-4);
                    }
                    swipe->set_tiles(bn::sprite_items::swipe.tiles_item(), frame);
                    swipe->set_position(bn::fixed(box.x) - cam, box.y - 80);
                    swipe->set_horizontal_flip(box.x < fx || (box.x == fx && f.facing < 0));
                    ++used;
                }
            }
            for(int k = used; k < 2; ++k) { v.swipes[k].reset(); }

            // the shot fox thief is holding floats above the head until it is thrown
            if(f.has_stored)
            {
                const ProjArt art = projectile_art(f.stored.kind);
                if(!v.held)
                {
                    v.held = art.item->create_sprite(0, 0, 0);
                    v.held->set_bg_priority(0);
                    v.held->set_z_order(-6);
                }
                v.held->set_position(bn::fixed(fx) - cam, fy - s.body_h - 12 - 80);
                v.held->set_horizontal_flip(f.facing < 0);
            }
            else { v.held.reset(); }
        }

        // ---------------------------------------------------------------------------- data

        int _phase = PH_OFF;
        int _timer = 0;
        int _banner_timer = 0;
        int _result = 0;
        bn::fixed _wipe = 0;
        bn::fixed _cam_x = 0;

        const StageDef* _stage = nullptr;
        const FighterDef* _player = nullptr;
        const FighterDef* _enemy = nullptr;
        Fight _fight;

        bn::optional<bn::regular_bg_ptr> _bgs[3];
        bn::vector<bn::sprite_ptr, FLOOR_TILES> _floor;
        bn::vector<Piece, 24> _pieces;
        bn::optional<FighterView> _views[2];
        bn::optional<bn::sprite_ptr> _proj[MAX_PROJECTILES];
        bn::optional<bn::sprite_ptr> _fx[MAX_EFFECTS];
        bn::vector<bn::sprite_ptr, 12> _blocks;
        bn::vector<bn::sprite_ptr, 12> _banner;
    };
}

#endif
