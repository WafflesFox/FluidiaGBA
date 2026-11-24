#ifndef BATTLE_LOGIC_H
#define BATTLE_LOGIC_H
// THE RULES OF THE BATTLE (plain C++)
//
//   A = normal attacks (jab, smashes, aerials)      B = special attacks (water gun, aqua jets, fox thief)
//   D-PAD + A or B picks the move.  R = jump.  L = shield.
//
// HOW MOVES WORK: a move is just DATA (see battle_moves.h). It is a little timeline, counted in frames
// from the moment the button is pressed. On that timeline a move can have:
//   - hitboxes     (melee: slaps, kicks, tail slams...)         -> Hitbox
//   - a drive      (the fighter flies: aqua jet)                -> Drive
//   - a shot       (something that flies away: water gun)       -> Shot
//   - an absorb    (catches enemy shots: fox thief)

namespace battle
{

    constexpr int FP = 16;
    constexpr int fp(double pixels) { return int(pixels * FP); }     // fp(1.5) = 1.5 pixels, as a fixed value

    // ---- tuning that applies to every fighter ----
    constexpr int SMASH_MAX = 1000;            // a full Final Smash gauge
    constexpr int SMASH_HIT_GAIN = 3;         // charge gained per point of damage dealt
    constexpr int SMASH_HURT_GAIN = 5;        // charge gained per point of damage taken
    constexpr int SMASH_PASSIVE_FRAMES = 30;    // +1 charge every N frames while fighting
    constexpr int SHIELD_MAX = 1000;            // a full shield
    constexpr int SHIELD_DRAIN = 4;             // lost every frame the shield is held (250 frames = about 4 seconds)
    constexpr int SHIELD_REGEN = 2;             // regained every frame it is not held (500 frames = about 8 seconds)
    constexpr int SHIELD_REACTIVATE = 250;      // once it BREAKS (hits 0) it works again after refilling this much
    constexpr int SHIELD_MAX_REDUCTION = 80;    // percent of damage removed by a FULL shield. Never 100: you still take chip damage.
    constexpr int SHIELD_HIT_COST = 10;         // shield lost per point of damage taken while guarding
    constexpr int SHIELD_PUSH = 3;              // a blocked hit pushes the guard back by (its knockback / this)
    constexpr int HURT_FRAMES = 10;             // flinch length when a hit does not say how long it stuns
    constexpr int DROP_FRAMES = 12;             // how long you fall through a platform
    constexpr int DROP_HOLD = 6;                // ...after holding DOWN this many frames (a quick DOWN + A/B is a down move instead)
    constexpr int BODY_MARGIN = 8;              // how close to the edge of the stage a fighter can stand
    constexpr int TIME_LIMIT = 60 * 300;         // 90 seconds, then whoever has more HP wins
    constexpr int KB_HP_BONUS = 60;             // KNOCKBACK: hits send you this many % further when you are almost dead
    constexpr int DRIVE_EXIT_PERCENT = 25;      // speed that is kept when a dash ends (the rest is thrown away so you don't keep sliding)
    constexpr int ABSORB_MARGIN = 6;            // fox thief catches shots this many pixels outside the body too
    constexpr int MAX_PROJECTILES = 8;
    constexpr int MAX_PLATFORMS = 6;
    constexpr int MAX_HITBOXES = 4;             // per move (raise it if a move needs more)
    constexpr int MAX_EFFECTS = 4;              // hit sparks on screen at once
    constexpr int PARTNER_SPACING = 24;         // a partner starts this many pixels behind the player
    constexpr int GRAB_DISTANCE = 16;           // a grabbed fighter is held this many pixels in front of the one who grabbed
    constexpr int GRAB_RELEASE_STUN = 14;       // a grabbed fighter that is let go without a hit (the grabber was interrupted)
    constexpr int POGO_RECOVERY = 10;           // frames of a move that are left after it bounced off somebody (pogo)

    // ============================================================================================
    // DATA: what a move is, what a fighter is, what a stage is
    // ============================================================================================

    enum { PROJ_WATER = 0, PROJ_EXPLOSION, PROJ_JUNK_PENCIL, PROJ_JUNK_APPLE, PROJ_JUNK_BAT, PROJ_JUNK_LAPTOP, PROJ_JUNK_BALL };    // which picture a shot uses (battle.h turns this into a sprite)
    enum { FX_SPARK = 0 };                      // little effects (only the hit spark for now)
    enum FinalSmashId { FS_NONE = 0, FS_WAFFLE_FIRE };

    // Which pose to show during a move. You have no attack sprites yet, so battle.h shows stand-in frames.
    // When you draw real ones, only pick_frame() in battle.h needs to change.
    enum
    {
        ANIM_STAND, ANIM_SLAP, ANIM_TAIL, ANIM_TAIL_UP, ANIM_KICK, ANIM_SPIN, ANIM_SLAM,
        ANIM_SHOOT, ANIM_JET_SIDE, ANIM_JET_UP, ANIM_JET_DOWN, ANIM_STANCE, ANIM_RTS, ANIM_SNEP_PUNCH,
        // Filip (snep)
        ANIM_SNEP_JAB, ANIM_SNEP_HEAVY, ANIM_SNEP_FLIP, ANIM_SNEP_DSMASH, ANIM_SNEP_COUNTER, ANIM_SNEP_COUNTER_GRAB,
        ANIM_SNEP_SPIN_THROW, ANIM_SNEP_THROW_UP, ANIM_SNEP_LEAP, ANIM_SNEP_SCRATCH, ANIM_SNEP_DAIR,
    };

    // Every button + direction combination is a SLOT. A fighter's MoveSet says which move sits in each slot.
    //   B alone / B + left-right / B + up / B + down        A on the ground                      A in the air
    enum Slot
    {
        SLOT_SPECIAL, SLOT_SPECIAL_SIDE, SLOT_SPECIAL_UP, SLOT_SPECIAL_DOWN,
        SLOT_JAB, SLOT_FSMASH, SLOT_USMASH, SLOT_DSMASH,
        SLOT_NAIR, SLOT_FAIR, SLOT_BAIR, SLOT_UAIR, SLOT_DAIR,
        SLOT_COUNT
    };

    enum : unsigned
    {
        MF_AIR_ONCE     = 1u << 0,      // can only be used once per jump (so up-specials can't be chained to fly away)
        MF_FIRE_ON_LAND = 1u << 1,      // the move's shot is fired when the fighter touches the ground (the dair explosion)
        MF_USE_STORED   = 1u << 2,      // the move's shot is the one fox thief stole, not its own
        MF_ORBIT_HELD   = 1u << 3,      // the stolen shot is drawn spinning around the fighter until it is thrown (battle.h)
    };

    // A melee hit. Every hitbox of a move can hit the opponent ONCE per use of the move.
    // Everything is set with the chained helpers, so a move reads like a sentence (see battle_moves.h):
    //     Hitbox().frames(4, 8).at(16, 14).size(16, 12).dmg(2).launch(fp(0.4), 0).stun(14)
    struct Hitbox
    {
        int start = 0, end = 0;     // active on frames start .. end-1 of the move (frame 0 = right after the button press)
        int x = 0, y = 0;           // the middle of the box: x = pixels IN FRONT of the fighter (negative = behind),
                                    //                        y = pixels above the feet
        int w = 10, h = 10;         // size in pixels
        int damage = 1;
        int kb_x = 0, kb_y = 0;     // launch speed (fp). kb_x pushes AWAY from the attacker. kb_y: negative = up, positive = down
        int hitstun = 0;            // frames the victim can't act (0 = HURT_FRAMES)
        bool grab = false;          // instead of damage, this hitbox GRABS the victim (the move's on_grab move takes over)
        bool keep_grab = false;     // on a victim that is grabbed: damage only, they stay held (the scratches)
        int pogo = 0;               // >0: the attacker bounces off the victim with this speed (fp), like a pogo stick
        bool absolute = false;      // x is NOT relative to where the fighter faces: negative = the LEFT of the screen

        constexpr Hitbox frames(int s, int e) const { Hitbox b = *this; b.start = s; b.end = e; return b; }
        constexpr Hitbox at(int px, int py) const   { Hitbox b = *this; b.x = px; b.y = py; return b; }
        constexpr Hitbox size(int pw, int ph) const { Hitbox b = *this; b.w = pw; b.h = ph; return b; }
        constexpr Hitbox dmg(int d) const           { Hitbox b = *this; b.damage = d; return b; }
        constexpr Hitbox launch(int kx, int ky) const { Hitbox b = *this; b.kb_x = kx; b.kb_y = ky; return b; }
        constexpr Hitbox stun(int frames_) const    { Hitbox b = *this; b.hitstun = frames_; return b; }
        constexpr Hitbox grabs() const              { Hitbox b = *this; b.grab = true; return b; }
        constexpr Hitbox keeps_grab() const         { Hitbox b = *this; b.keep_grab = true; return b; }
        constexpr Hitbox bounces(int speed_fp) const { Hitbox b = *this; b.pogo = speed_fp; return b; }
        constexpr Hitbox left_right() const         { Hitbox b = *this; b.absolute = true; return b; }
    };

    // Something that flies away (or just appears and stays, like an explosion).
    struct Shot
    {
        int kind = PROJ_WATER;      // PROJ_...
        int damage = 1;
        int speed = 0;              // fp per frame (0 = stays where it appears)
        int life = 30;              // frames before it disappears (speed x life = range)
        int w = 8, h = 8;           // hitbox size in pixels
        int dx = 0, dy = 0;         // where it appears: pixels in front of the fighter's middle / above the feet
        int kb_x = 0, kb_y = 0;     // launch, same as Hitbox
        int hitstun = 0;
        bool absorbable = true;     // fox thief can steal it
        bool persist = false;       // stays around after it hit (explosions); it still only hits once
        int vy = 0;                 // fp per frame, up/down speed at the start (negative = up)
        int grav = 0;               // fp per frame: pulls the shot down (thrown objects fly in an arc). 0 = flies straight
        const char* label = nullptr;    // shown as text instead of a picture (until there are pictures for it)
        int boom = 0;               // > 0: when it hits (or lands) it explodes, with this much damage

        constexpr Shot of(int k) const              { Shot s = *this; s.kind = k; return s; }
        constexpr Shot dmg(int d) const             { Shot s = *this; s.damage = d; return s; }
        constexpr Shot fly(int speed_fp, int frames_) const { Shot s = *this; s.speed = speed_fp; s.life = frames_; return s; }
        constexpr Shot size(int pw, int ph) const   { Shot s = *this; s.w = pw; s.h = ph; return s; }
        constexpr Shot spawn(int pdx, int pdy) const { Shot s = *this; s.dx = pdx; s.dy = pdy; return s; }
        constexpr Shot launch(int kx, int ky) const { Shot s = *this; s.kb_x = kx; s.kb_y = ky; return s; }
        constexpr Shot stun(int frames_) const      { Shot s = *this; s.hitstun = frames_; return s; }
        constexpr Shot steal_proof() const          { Shot s = *this; s.absorbable = false; return s; }
        constexpr Shot lingers() const              { Shot s = *this; s.persist = true; return s; }
        constexpr Shot arc(int vx_fp, int vy_fp, int gravity_fp, int frames_) const
        {
            Shot s = *this; s.speed = vx_fp; s.vy = vy_fp; s.grav = gravity_fp; s.life = frames_; return s;
        }
        constexpr Shot labeled(const char* text) const { Shot s = *this; s.label = text; return s; }
        constexpr Shot explodes(int boom_damage) const { Shot s = *this; s.boom = boom_damage; return s; }
    };

    // The fighter FLIES for a while: gravity is off and the speed is forced (aqua jet).
    struct Drive
    {
        int start = 0, end = 0;     // active on frames start .. end-1 (end <= start = no drive)
        int vx = 0, vy = 0;         // fp per frame. vx points where the fighter faces. vy: negative = up
        int homing = 0;             // 0..100: how much it steers up/down toward the opponent (only if they are in front)
        bool stop_on_hit = false;   // the dash ends the moment it hits something

        constexpr Drive frames(int s, int e) const  { Drive d = *this; d.start = s; d.end = e; return d; }
        constexpr Drive speed(int sx, int sy) const { Drive d = *this; d.vx = sx; d.vy = sy; return d; }
        constexpr Drive steer(int percent) const    { Drive d = *this; d.homing = percent; return d; }
        constexpr Drive stops_on_hit() const        { Drive d = *this; d.stop_on_hit = true; return d; }
    };

    // A whole move.   Move("name", total frames, pose).hit(...).hit(...).dash(...)...
    struct Move
    {
        const char* name = "";
        int duration = 20;          // total length in frames, recovery included
        int anim = ANIM_STAND;
        unsigned flags = 0;         // MF_...
        Hitbox hits[MAX_HITBOXES] = {};
        int hit_count = 0;
        Drive drive = {};
        int shot_frame = -1;        // the frame the shot is fired (-1 = this move fires nothing at a set time)
        Shot shot = {};
        int land_frame = 0;         // MF_FIRE_ON_LAND: the move jumps to this frame when it lands (so only the recovery is left)
        int absorb_start = 0, absorb_end = 0;   // frames where enemy shots that touch the fighter are stolen
        int power = 100;            // MF_USE_STORED: % of the stolen shot's damage and speed
        const Move* alt = nullptr;  // used INSTEAD of this move while the fighter is holding a stolen shot
        const Shot* shot_list = nullptr;    // fires_random: one of these is picked each time
        int shot_list_count = 0;
        int leap_frame = -1;        // leaps(): at this frame the fighter is thrown off the ground (a jump that can't be steered)
        int leap_vx = 0, leap_vy = 0;
        int armor_start = 0, armor_end = 0, armor_limit = 0;   // super armor: hits of at most armor_limit damage don't flinch
        int counter_start = 0, counter_end = 0;                // counter window: a melee hit in it is caught instead
        const Move* on_counter_melee = nullptr;                // ...and this move starts (the victim is grabbed)
        const Move* on_absorb = nullptr;                       // a shot that was stolen (absorbs) starts this move at once
        const Move* on_grab = nullptr;                         // a hitbox with grabs() connected: this move takes over
        int land_end = 0;           // ends_on_landing: when a downward dash touches the ground, jump to this frame
        int final_smash = FS_NONE; // >0: landing a melee hit with this move starts this Final Smash

        constexpr Move() = default;
        constexpr Move(const char* n, int total_frames, int pose) : name(n), duration(total_frames), anim(pose) {}

        constexpr Move hit(const Hitbox& h) const
        {
            Move m = *this;
            m.hits[m.hit_count] = h;            // (more than MAX_HITBOXES = a compile error: raise MAX_HITBOXES)
            ++m.hit_count;
            return m;
        }
        constexpr Move dash(const Drive& d) const { Move m = *this; m.drive = d; return m; }
        constexpr Move fires(int frame, const Shot& s) const { Move m = *this; m.shot_frame = frame; m.shot = s; return m; }
        constexpr Move fires_on_landing(const Shot& s, int frame_after_landing) const
        {
            Move m = *this; m.shot = s; m.land_frame = frame_after_landing; m.flags |= MF_FIRE_ON_LAND; return m;
        }
        constexpr Move throws_stored(int frame, int power_percent) const
        {
            Move m = *this; m.shot_frame = frame; m.power = power_percent; m.flags |= MF_USE_STORED; return m;
        }
        constexpr Move absorbs(int s, int e) const { Move m = *this; m.absorb_start = s; m.absorb_end = e; return m; }
        constexpr Move swaps_when_holding(const Move* other) const { Move m = *this; m.alt = other; return m; }
        constexpr Move once_per_jump() const { Move m = *this; m.flags |= MF_AIR_ONCE; return m; }

        // ---- for the moves of Filip ----
        constexpr Move with_anim(int pose) const { Move m = *this; m.anim = pose; return m; }     // the same move with another picture
        constexpr Move fires_random(int frame, const Shot* list, int how_many) const
        {
            Move m = *this; m.shot_frame = frame; m.shot_list = list; m.shot_list_count = how_many; m.shot = list[0]; return m;
        }
        constexpr Move leaps(int frame, int vx_fp, int vy_fp) const
        {
            Move m = *this; m.leap_frame = frame; m.leap_vx = vx_fp; m.leap_vy = vy_fp; return m;
        }
        constexpr Move super_armor(int from, int to, int max_damage) const
        {
            Move m = *this; m.armor_start = from; m.armor_end = to; m.armor_limit = max_damage; return m;
        }
        constexpr Move counters_melee(int from, int to, const Move* then) const
        {
            Move m = *this; m.counter_start = from; m.counter_end = to; m.on_counter_melee = then; return m;
        }
        constexpr Move after_absorb(const Move* then) const { Move m = *this; m.on_absorb = then; return m; }
        constexpr Move after_grab(const Move* then) const { Move m = *this; m.on_grab = then; return m; }
        constexpr Move ends_on_landing(int frame) const { Move m = *this; m.land_end = frame; return m; }
        constexpr Move orbits_held() const { Move m = *this; m.flags |= MF_ORBIT_HELD; return m; }
        constexpr Move starts_final_smash(int id) const { Move m = *this; m.final_smash = id; return m; }
    };

    // Which move sits in which slot. nullptr = this fighter simply has no such move.
    struct MoveSet
    {
        const Move* slot[SLOT_COUNT] = {};

        constexpr MoveSet set(int which, const Move* m) const
        {
            MoveSet s = *this;
            s.slot[which] = m;
            return s;
        }
    };

    struct FighterStats
    {
        int max_hp;
        int walk_speed;         // fp per frame
        int ground_accel;       // fp per frame, how fast you reach full speed
        int air_accel;
        int friction;           // fp per frame, how fast you stop on the ground
        int jump_power;         // fp: the speed you leave the ground with
        int double_jump_power;
        int max_jumps;          // 2 = one jump + one in the air
        int gravity;            // fp per frame
        int max_fall;           // fp
        int fast_fall;          // fp, the fall speed while holding DOWN
        int body_w, body_h;     // hurtbox in pixels (feet at the bottom middle)
        int defense;            // percent of damage ALWAYS removed (0 = none)
        int ai_skill;           // 0..100: how well the computer plays this fighter
        const MoveSet* moves;   // every move this fighter knows (see battle_moves.h)
        int weight = 100;       // 100 = normal. Heavier fighters (150) get launched less, lighter ones (70) more
        int phys_pct = 100;     // % of damage dealt by A moves (jab, smashes, aerials). 100 = normal, 120 = 20% stronger
        int magic_pct = 100;    // % of damage dealt by B moves (water gun, aqua jets...)
        const Move* final_smash_move = nullptr; // starter move launched when SELECT is pressed with full meter
    };

    // A small bonus from a companion that follows you but does not fight (main.cpp turns NPC traits into one of these).
    // Every field is an ADDITION: 0 = no change. Buffs from several traits simply add up.
    struct Buff
    {
        int phys_pct = 0;       // +% damage of A moves
        int magic_pct = 0;      // +% damage of B moves
        int speed_pct = 0;      // +% walking speed (and the acceleration that goes with it)
        int jump_pct = 0;       // +% jump speed (about twice that in height)
        int defense = 0;        // + percent of damage that is always removed
    };

    // The stats with a buff on top. The result is a COPY: Fight keeps a pointer to the stats it was given, so the
    // copy has to stay alive for the whole fight (battle.h keeps it in the Battle object).
    inline FighterStats with_buff(FighterStats s, const Buff& b)
    {
        s.phys_pct += b.phys_pct;
        s.magic_pct += b.magic_pct;
        s.walk_speed = s.walk_speed * (100 + b.speed_pct) / 100;
        s.ground_accel = s.ground_accel * (100 + b.speed_pct) / 100;
        s.air_accel = s.air_accel * (100 + b.speed_pct) / 100;
        s.jump_power = s.jump_power * (100 + b.jump_pct) / 100;
        s.double_jump_power = s.double_jump_power * (100 + b.jump_pct) / 100;
        s.defense += b.defense;
        return s;
    }

    struct Platform
    {
        int x, y, w;            // x = left edge, y = the surface you stand on, w = width, in pixels
    };

    struct StageGeom
    {
        int length;             // how wide the stage is, in pixels (240 = exactly one screen, no scrolling)
        int ground_y;           // y of the floor surface
        const Platform* platforms;
        int platform_count;
        int spawn_player;       // x where the player starts (0 = automatic)
        int spawn_enemy;
    };

    // ============================================================================================
    // THE FIGHT
    // ============================================================================================

    enum State { ST_IDLE, ST_WALK, ST_AIR, ST_ATTACK, ST_SHIELD, ST_HURT, ST_KO, ST_GRABBED };
    enum DebugAiMode { DBG_AI_NORMAL = 0, DBG_AI_IDLE, DBG_AI_FORCE };   // GRABBED: held by somebody, can't act

    // what a fighter "pressed" this frame (the player's buttons, or the computer's decision)
    struct Input
    {
        int move = 0;           // -1 left, 0 nothing, +1 right (held)
        bool up = false;        // held
        bool down = false;      // held
        bool jump = false;      // pressed this frame
        bool attack = false;    // A pressed this frame
        bool special = false;   // B pressed this frame
        bool shield = false;    // held
        bool final_smash = false;
    };

    struct Fighter
    {
        const FighterStats* stats = nullptr;
        int x16 = 0, y16 = 0;       // the middle of the feet
        int vx = 0, vy = 0;
        int facing = 1;             // +1 right, -1 left
        bool on_ground = false;
        int support = -1;           // what you stand on: -1 = the floor, otherwise the platform's number
        int jumps_left = 0;
        int hp = 0;
        int shield = SHIELD_MAX;
        bool shield_locked = false; // the shield broke and is still recharging
        int smash = 0;
        int state = ST_AIR;
        int timer = 0;              // frames left of a flinch
        int drop_timer = 0;
        int down_hold = 0;          // how long DOWN has been held on a platform

        // the move being performed (only while state == ST_ATTACK)
        const Move* move = nullptr;
        int move_frame = 0;         // frames since the move began
        unsigned hit_done = 0;      // bit k set = hitbox k already hit
        bool driving = false;       // flying right now (aqua jet)
        bool drive_cut = false;     // the dash was ended early (it hit, or it landed)
        bool land_fired = false;
        unsigned air_used = 0;      // bit per slot: used in this jump already (MF_AIR_ONCE)
        bool move_magic = false;    // the move being performed is a B move (uses magic_pct instead of phys_pct)
        int dmg_carry[2] = {};      // leftover from damage buffs: +20% of a 2-damage hit is 0.4, so the missing bits are
                                    // saved here and paid out later (index 0 = physical, 1 = magic)

        // fox thief
        bool has_stored = false;
        Shot stored = {};

        // grabs: grabbed_by = the fighter holding me (-1 = nobody), holding = the fighter I am holding
        int grabbed_by = -1;
        int holding = -1;
    };

    struct Projectile
    {
        bool active = false;
        int id = 0;
        int owner = 0;
        int x16 = 0, y16 = 0;       // the middle of it
        int vx = 0;
        int vy = 0;
        int age = 0;
        int life = 0;
        bool hit_done = false;
        Shot shot = {};
    };

    // little pictures that play once (hit sparks). battle.h draws them.
    struct Effect
    {
        bool active = false;
        int kind = 0;
        int x16 = 0, y16 = 0;
        int age = 0;
        int life = 1;
    };

    // things that happened this frame, so battle.h can play sounds
    struct Events
    {
        bool shot = false;          // something was fired / thrown
        bool swing = false;         // a melee move started
        bool dash = false;          // an aqua jet took off
        bool absorb = false;        // fox thief stole a shot
        bool boom = false;          // the dair explosion
        bool hit = false;
        bool blocked = false;       // a hit that landed on a shield
        bool jump = false;
        bool ko = false;
        bool hitsmash = false;
    };

    // The computer's memory
    struct Ai
    {
        int handled_projectile = -1;    // the last incoming shot it already made a decision about
        int absorb_handled = -1;
        int shield_hold = 0;
        int drop_hold = 0;
        int strafe = 0;
        int strafe_timer = 0;
        int mode_timer = 0;
        int last_react = -1000;         // the frame it last decided whether to guard against a melee move
        bool rush = false;              // true = walks up and fights close, false = keeps its distance and shoots
    };

    struct Rect { int x, y, w, h; };    // a box: x and y are its MIDDLE, in pixels

    inline int abs_i(int v) { return v < 0 ? -v : v; }
    inline int sign_i(int v) { return v < 0 ? -1 : (v > 0 ? 1 : 0); }

    inline bool overlaps(const Rect& a, const Rect& b)
    {
        return abs_i(a.x - b.x) * 2 < a.w + b.w && abs_i(a.y - b.y) * 2 < a.h + b.h;
    }

    inline bool has_move(const FighterStats& s, int which) { return s.moves != nullptr && s.moves->slot[which] != nullptr; }

    struct Fight
    {
        StageGeom stage = {};
        Fighter f[3];               // f[0] = the player, f[1] = the opponent, f[2] = the player's partner (only if count == 3)
        int count = 2;              // 2 = one on one, 3 = you and a partner against the opponent
        Projectile proj[MAX_PROJECTILES];
        Effect fx[MAX_EFFECTS];
        Ai ai[3];
        Events ev;
        unsigned rng = 1;
        int frame = 0;
        int winner = -1;            // -1 = still fighting, 0 = the player won, 1 = the opponent won
        int pending_final_smash = -1;
        int pending_final_smash_victim = -1;
        int pending_final_smash_id = FS_NONE;
        int debug_ai_mode = DBG_AI_NORMAL;
        int debug_force_slot = SLOT_SPECIAL;
        int debug_cd = 0;
        int next_projectile_id = 0;

        void apply_final_smash_hit(int attacker_idx, int victim_idx, int damage, int kb_x, int kb_y, int stun)
        {
            ev = Events();
            for(int i = 0; i < count; ++i)
            {
                release_grab(i);
                cancel_move(f[i]);
                if(f[i].state != ST_KO) { f[i].state = f[i].on_ground ? ST_IDLE : ST_AIR; }
            }
            for(Projectile& p : proj) { p.active = false; }

            Fighter& attacker = f[attacker_idx];
            Fighter& victim = f[victim_idx];
            if(victim.state == ST_KO) { return; }

            int dir = sign_i(victim.x16 - attacker.x16);
            if(dir == 0) { dir = attacker.facing; }
            attacker.facing = dir;

            if(victim.state == ST_SHIELD) { victim.state = ST_IDLE; }   // Final Smashes break through guard

            hit(victim, damage, kb_x, kb_y, stun, dir, victim.x16, victim.y16 - (victim.stats->body_h * FP) / 2, nullptr);
            if(victim.hp <= 0 && winner < 0)
            {
                winner = team_of(attacker_idx) == 0 ? 0 : 1;
            }
        }

        // `partner` (optional) is a computer-controlled fighter on the PLAYER's side: a 2 against 1 fight.
        // The stats are only pointed at, not copied: they have to stay alive for the whole fight.
        void start(const StageGeom& geometry, const FighterStats& player, const FighterStats& enemy, unsigned seed,
                   const FighterStats* partner = nullptr)
        {
            stage = geometry;
            count = partner ? 3 : 2;
            rng = seed * 2654435761u + 977u;
            if(rng == 0) { rng = 1; }
            frame = 0;
            winner = -1;
            pending_final_smash = -1;
            pending_final_smash_victim = -1;
            pending_final_smash_id = FS_NONE;
            debug_ai_mode = DBG_AI_NORMAL;
            debug_force_slot = SLOT_SPECIAL;
            debug_cd = 0;
            next_projectile_id = 0;
            for(Projectile& p : proj) { p = Projectile(); }
            for(Effect& e : fx) { e = Effect(); }
            for(Ai& brain : ai) { brain = Ai(); }

            const int player_x = geometry.spawn_player > 0 ? geometry.spawn_player : geometry.length * 3 / 10;
            const FighterStats* stats[3] = { &player, &enemy, partner };
            const int spawn[3] =
            {
                player_x,
                geometry.spawn_enemy > 0 ? geometry.spawn_enemy : geometry.length * 7 / 10,
                player_x - PARTNER_SPACING >= BODY_MARGIN ? player_x - PARTNER_SPACING : player_x + PARTNER_SPACING,
            };

            for(int i = 0; i < 3; ++i) { f[i] = Fighter(); }              // (an unused slot stays empty and is never touched)
            for(int i = 0; i < count; ++i)
            {
                f[i].stats = stats[i];
                f[i].x16 = spawn[i] * FP;
                f[i].y16 = geometry.ground_y * FP;
                f[i].on_ground = true;
                f[i].hp = stats[i]->max_hp;
                f[i].jumps_left = stats[i]->max_jumps;
                f[i].facing = team_of(i) == 0 ? 1 : -1;
                f[i].state = ST_IDLE;
            }
        }

        // Advance the fight by one frame. in[0] = the player's input, in[1] = the opponent's, in[2] = the partner's
        // (in[2] is only read when there is a partner).
        void step(const Input* in)
        {
            ev = Events();
            for(int i = 0; i < count; ++i)
            {
                update_fighter(i, in[i]);
                if(pending_final_smash >= 0) { return; }   // pause frame immediately for cutscene
            }
            resolve_melee();
            if(pending_final_smash >= 0) { return; }
            for(int i = 0; i < count; ++i) { advance_move(i); }
            update_grabs();
            update_projectiles();
            for(Effect& e : fx) { if(e.active && ++e.age >= e.life) { e.active = false; } }
            ++frame;

            if(winner < 0 && frame % SMASH_PASSIVE_FRAMES == 0)
            {
                for(int i = 0; i < count; ++i)
                {
                    if(f[i].stats->final_smash_move != nullptr && f[i].state != ST_KO && f[i].smash < SMASH_MAX)
                    {
                        ++f[i].smash;
                    }
                }
            }

            if(winner < 0)
            {
                if(f[0].hp <= 0)      { winner = 1; }
                else if(f[1].hp <= 0) { winner = 0; }
                else if(frame >= TIME_LIMIT) { winner = f[0].hp > f[1].hp ? 0 : 1; }
            }
        }

        // Teams: the player (0) and the partner (2) against the opponent (1).
        static int team_of(int i) { return i == 1 ? 1 : 0; }

        // The fighter that fighter i is fighting. The player and the partner both fight the opponent. The opponent fights
        // whichever of the two is closest (a knocked-out one is ignored).
        int foe_of(int i) const
        {
            if(team_of(i) == 0) { return 1; }
            if(count < 3) { return 0; }
            const bool first_down = f[0].state == ST_KO;
            const bool partner_down = f[2].state == ST_KO;
            if(first_down != partner_down) { return first_down ? 2 : 0; }
            return abs_i(f[0].x16 - f[i].x16) <= abs_i(f[2].x16 - f[i].x16) ? 0 : 2;
        }

        // Which slot do these buttons mean?  (public: the computer and the tests use it too)
        static int pick_slot(const Input& in, const Fighter& p)
        {
            const bool up = in.up && !in.down;
            const bool down = in.down && !in.up;

            if(in.special)                                  // B: the specials, the same on the ground and in the air
            {
                if(up)           { return SLOT_SPECIAL_UP; }
                if(down)         { return SLOT_SPECIAL_DOWN; }
                if(in.move != 0) { return SLOT_SPECIAL_SIDE; }
                return SLOT_SPECIAL;
            }

            if(p.on_ground)                                 // A on the ground: jab and smashes
            {
                if(up)           { return SLOT_USMASH; }
                if(down)         { return SLOT_DSMASH; }
                if(in.move != 0) { return SLOT_FSMASH; }
                return SLOT_JAB;
            }

            if(up)               { return SLOT_UAIR; }      // A in the air: aerials ("forward" and "back" depend on where you face)
            if(down)             { return SLOT_DAIR; }
            if(in.move != 0)     { return in.move == p.facing ? SLOT_FAIR : SLOT_BAIR; }
            return SLOT_NAIR;
        }

        // is hitbox k of fighter i's move active RIGHT NOW? (battle.h uses it to draw the swipes)
        bool hitbox_now(int i, int k, Rect& out) const
        {
            const Fighter& p = f[i];
            if(p.state != ST_ATTACK || !p.move || k >= p.move->hit_count) { return false; }
            const Hitbox& h = p.move->hits[k];
            if(p.move_frame < h.start || p.move_frame >= h.end) { return false; }
            out = Rect{ p.x16 / FP + (h.absolute ? h.x : p.facing * h.x), p.y16 / FP - h.y, h.w, h.h };
            return true;
        }

        // The computer decides what to press. Use the result as in[1] (or in[0] to let it play for you).
        Input think(int self)
        {
            Input in;
            const Fighter& me = f[self];
            const Fighter& op = f[foe_of(self)];
            Ai& brain = ai[self];
            if(winner >= 0 || me.state == ST_KO) { return in; }

            if(self == 1 && debug_ai_mode == DBG_AI_IDLE)
            {
                return in;
            }
            if(self == 1 && debug_ai_mode == DBG_AI_FORCE)
            {
                const bool can_act_dbg = me.state == ST_IDLE || me.state == ST_WALK || me.state == ST_AIR;
                if(!can_act_dbg) { return in; }
                if(debug_cd > 0) { --debug_cd; return in; }

                const int toward_dbg = op.x16 >= me.x16 ? 1 : -1;
                if(debug_force_slot == SLOT_COUNT)
                {
                    if(!me.stats->final_smash_move || op.state == ST_KO) { return in; }
                    if(me.smash < SMASH_MAX) { f[self].smash = SMASH_MAX; }
                    const int adx_dbg = abs_i(op.x16 - me.x16) / FP;
                    if(adx_dbg > 44 || me.facing != toward_dbg) { in.move = toward_dbg; return in; }
                    in.final_smash = true;
                    debug_cd = 25;
                    return in;
                }

                const int slot = debug_force_slot;
                if(!has_move(*me.stats, slot)) { return in; }

                const bool is_aerial = slot >= SLOT_NAIR && slot <= SLOT_DAIR;
                if(is_aerial && me.on_ground)
                {
                    in.jump = true;
                    return in;
                }

                const int wanted_face = (slot == SLOT_BAIR) ? -toward_dbg : toward_dbg;
                if(me.facing != wanted_face && slot != SLOT_SPECIAL_SIDE && slot != SLOT_FSMASH)
                {
                    in.move = wanted_face;
                    return in;
                }

                switch(slot)
                {
                    case SLOT_SPECIAL:      in.special = true; break;
                    case SLOT_SPECIAL_SIDE: in.special = true; in.move = toward_dbg; break;
                    case SLOT_SPECIAL_UP:   in.special = true; in.up = true; break;
                    case SLOT_SPECIAL_DOWN: in.special = true; in.down = true; break;
                    case SLOT_JAB:          in.attack = true; break;
                    case SLOT_FSMASH:       in.attack = true; in.move = toward_dbg; break;
                    case SLOT_USMASH:       in.attack = true; in.up = true; break;
                    case SLOT_DSMASH:       in.attack = true; in.down = true; break;
                    case SLOT_NAIR:         in.attack = true; break;
                    case SLOT_FAIR:         in.attack = true; in.move = me.facing; break;
                    case SLOT_BAIR:         in.attack = true; in.move = -me.facing; break;
                    case SLOT_UAIR:         in.attack = true; in.up = true; break;
                    case SLOT_DAIR:         in.attack = true; in.down = true; break;
                    default: break;
                }
                debug_cd = 22;
                return in;
            }

            const FighterStats& s = *me.stats;
            const int dx = (op.x16 - me.x16) / FP;            // + = the opponent is on my right
            const int dy = (op.y16 - me.y16) / FP;            // + = the opponent is below me
            const int adx = abs_i(dx);
            const int ady = abs_i(dy);
            const int toward = dx > 0 ? 1 : -1;
            const bool can_act = me.state == ST_IDLE || me.state == ST_WALK || me.state == ST_AIR;

            // falling through a platform means holding DOWN for a few frames
            if(brain.drop_hold > 0) { --brain.drop_hold; in.down = true; return in; }

            // 0. If Final Smash is ready and within range, activate it!
            if(can_act && s.final_smash_move && me.smash >= SMASH_MAX && op.state != ST_KO && adx < 52 && ady < 32)
            {
                if(me.facing != toward) { in.move = toward; return in; }
                if(random_below(100) < 35 + s.ai_skill / 2)
                {
                    in.final_smash = true;
                    return in;
                }
            }

            // 1. is a shot coming at me? decide ONCE per shot whether to react (the better the skill, the more often)
            const Projectile* threat = nullptr;
            int threat_dist = 9999;
            const int my_mid_y = me.y16 - (s.body_h * FP) / 2;
            for(const Projectile& p : proj)
            {
                if(!p.active || team_of(p.owner) == team_of(self) || p.vx == 0) { continue; }
                const int pdx = (p.x16 - me.x16) / FP;
                const bool coming = (pdx > 0 && p.vx < 0) || (pdx < 0 && p.vx > 0);
                const int pdy = abs_i((p.y16 - my_mid_y) / FP);
                if(coming && abs_i(pdx) < 100 && pdy < s.body_h / 2 + p.shot.h / 2 + 4 && abs_i(pdx) < threat_dist)
                {
                    threat = &p;
                    threat_dist = abs_i(pdx);
                }
            }

            // 1a. fox thief: steal the shot right as it arrives (it takes a few frames to get ready)
            if(threat && threat_dist < 76 && threat->id != brain.absorb_handled)
            {
                brain.absorb_handled = threat->id;
                if(can_act && !me.has_stored && threat->shot.absorbable && has_move(s, SLOT_SPECIAL_DOWN) && random_below(100) < s.ai_skill / 2)
                {
                    brain.handled_projectile = threat->id;
                    in.special = true;
                    in.down = true;
                    return in;
                }
            }

            // 1b. otherwise guard or jump over it
            if(threat && threat->id != brain.handled_projectile)
            {
                brain.handled_projectile = threat->id;
                if(me.on_ground && random_below(100) < s.ai_skill)       { brain.shield_hold = 18 + random_below(10); }
                else if(me.jumps_left > 0 && random_below(100) < 45)     { in.jump = true; }
            }
            if(brain.shield_hold > 0)
            {
                --brain.shield_hold;
                in.shield = true;
                return in;                                                // while guarding, do nothing else
            }

            // 1c. is the opponent swinging at me? (decided once per swing, so a better player guards more often)
            if(me.on_ground && can_act && op.state == ST_ATTACK && op.move && op.move->hit_count > 0 && adx < 46 && ady < 34 &&
               frame - brain.last_react > 25)
            {
                brain.last_react = frame;
                if(random_below(100) < s.ai_skill / 2) { brain.shield_hold = 14 + random_below(12); }
            }
            if(brain.shield_hold > 0)
            {
                --brain.shield_hold;
                in.shield = true;
                return in;
            }

            // 2. holding a stolen shot? throw it back
            if(me.has_stored && can_act && has_move(s, SLOT_SPECIAL_DOWN) && ady < 26 && adx < 170 && random_below(100) < 6)
            {
                in.move = toward;
                in.special = true;
                in.down = true;
                return in;
            }

            // 3. movement. Sometimes it keeps its distance and shoots, sometimes it rushes in and fights close
            if(--brain.mode_timer <= 0)
            {
                brain.mode_timer = 90 + random_below(150);
                brain.rush = random_below(100) < 30 + s.ai_skill / 3;
            }
            const bool seeking_fs = me.stats->final_smash_move != nullptr && me.smash >= SMASH_MAX;
            const int wanted_range = seeking_fs ? 18 : (brain.rush ? 22 : 105);
            if(--brain.strafe_timer <= 0)
            {
                brain.strafe_timer = 25 + random_below(50);
                brain.strafe = random_below(3) - 1;
            }
            if(adx > wanted_range + 25)       { in.move = toward; }
            else if(adx < wanted_range - 45)  { in.move = -toward; }
            else                              { in.move = brain.strafe; }

            // 4. attacks
            if(can_act)
            {
                const bool grounded = me.on_ground;
                const bool facing_op = me.facing == toward;
                const int urge = 2 + s.ai_skill / 20 + (brain.rush ? 3 : 0);          // % chance per frame

                if(adx < 34 && ady < 30)                                              // close: melee
                {
                    if(random_below(100) < urge + 4)
                    {
                        const int r = random_below(100);
                        if(grounded)
                        {
                            // (a neutral press needs in.move = 0, or the direction the computer is walking turns it into a smash)
                            if(r < 40)                                 { if(facing_op) { in.move = 0; in.attack = true; } else { in.move = toward; } }   // slap combo
                            else if(r < 70)                            { in.move = toward; in.attack = true; }                                           // forward smash
                            else if(r < 85)                            { in.down = true; in.attack = true; }                                             // down smash
                            else                                       { if(facing_op) { in.move = 0; in.attack = true; } }
                        }
                        else
                        {
                            if(r < 40) { in.move = 0; in.attack = true; }                         // spin
                            else       { in.move = toward; in.attack = true; }                     // kick (or tail slap if they are behind)
                        }
                    }
                }
                else if(dy < -26 && adx < 30)                                          // they are above me
                {
                    const int r = random_below(100);
                    if(r < urge + 2)                                { in.up = true; in.attack = true; }                                     // up smash / up air
                    else if(r < urge + 4 && has_move(s, SLOT_SPECIAL_UP)) { in.up = true; in.special = true; }                                 // aqua jet up
                }
                else if(!grounded && dy > 24 && adx < 22)                              // they are below me and I am in the air
                {
                    if(random_below(100) < urge + 4) { in.down = true; in.attack = true; }  // dair
                }
                else if(ady < 26)                                                      // roughly level: shoot or dash at them
                {
                    const Move* jet = s.moves ? s.moves->slot[SLOT_SPECIAL_SIDE] : nullptr;
                    const Move* gun = s.moves ? s.moves->slot[SLOT_SPECIAL] : nullptr;
                    const int jet_range = !jet ? 0 : (jet->leap_frame >= 0 ? jet->leap_vx * 16 / FP + 12 : jet->drive.vx * (jet->drive.end - jet->drive.start) / FP + 12);
                    const int gun_range = (gun && gun->shot_frame >= 0) ? gun->shot.speed * gun->shot.life / FP : 0;

                    if(jet && adx > 40 && adx < jet_range && random_below(1000) < 6 + s.ai_skill / 10)
                    {
                        in.move = toward;                                              // aqua jet
                        in.special = true;
                    }
                    else if(gun_range > 0 && adx > 40 && adx < gun_range && random_below(100) < 3 + s.ai_skill / 12)
                    {
                        if(facing_op) { in.move = 0; in.special = true; }             // water gun (needs to face them first)
                        else          { in.move = toward; }
                    }
                }
            }

            // 5. get to a different height
            if(!in.attack && !in.special)
            {
                if(seeking_fs && me.on_ground && dy < -24 && random_below(100) < 12) { in.jump = true; }
                else if(me.on_ground && dy < -36 && random_below(100) < 5)                        { in.jump = true; }
                else if(me.on_ground && me.support >= 0 && dy > 36 && random_below(100) < 4) { brain.drop_hold = DROP_HOLD + 2; in.down = true; }
                else if(me.on_ground && random_below(1000) < 4)                              { in.jump = true; }
            }
            return in;
        }

    private:
        struct PendingHit
        {
            int attacker;
            int victim;                 // which fighter was touched (a hitbox can touch both enemies of a 2 against 1 fight)
            int index;                  // which hitbox of the attacker's move
            Hitbox box;                 // a copy, because the attacker's move can be cut short by the same frame's hits
            int dir;                    // +1: the victim is launched to the right
            int sx16, sy16;             // where the hit spark appears
            bool stops_dash;
        };

        int random_below(int limit)
        {
            rng ^= rng << 13;
            rng ^= rng >> 17;
            rng ^= rng << 5;
            return int(rng % unsigned(limit));
        }
        

        static int approach(int value, int target, int amount)
        {
            if(value < target) { value += amount; if(value > target) { value = target; } }
            else if(value > target) { value -= amount; if(value < target) { value = target; } }
            return value;
        }

        static Rect hurtbox_of(const Fighter& t)
        {
            return Rect{ t.x16 / FP, t.y16 / FP - t.stats->body_h / 2, t.stats->body_w, t.stats->body_h };
        }
        void update_fighter(int i, const Input& raw)
        {
            Fighter& p = f[i];
            const FighterStats& s = *p.stats;
            Input in = raw;
            if(winner >= 0 || p.state == ST_KO || p.state == ST_GRABBED) { in = Input(); }
            if(p.drop_timer > 0) { --p.drop_timer; }

            // ---- states that run by themselves ----
            bool floating = false;                                        // true while an aqua jet is flying
            if(p.state == ST_ATTACK)
            {
                if(p.move) { floating = run_move(i); }
                else       { p.state = p.on_ground ? ST_IDLE : ST_AIR; }  // (safety: an attack without a move)
            }
            else if(p.state == ST_HURT)
            {
                if(--p.timer <= 0) { p.state = p.on_ground ? ST_IDLE : ST_AIR; }
            }

            // ---- states where the fighter is in control ----
            const bool free_state = p.state == ST_IDLE || p.state == ST_WALK || p.state == ST_AIR || p.state == ST_SHIELD;
            bool steering = false;

            if(free_state)
            {
                bool started = false;
                if(in.final_smash && s.final_smash_move && p.smash >= SMASH_MAX)
                {
                    p.smash = 0;
                    if(in.move != 0) { p.facing = in.move; }
                    begin_move(p, s.final_smash_move, true);
                    if(s.final_smash_move->hit_count > 0 || s.final_smash_move->drive.end > s.final_smash_move->drive.start) { ev.swing = true; }
                    started = true;
                }
                else if((in.attack || in.special) && p.state != ST_SHIELD) { started = try_start_move(p, in); }

                if(!started)
                {
                    if(in.move != 0) { p.facing = in.move; }

                    if(in.jump && p.jumps_left > 0)                       // jumping also cancels the shield
                    {
                        p.vy = -(p.on_ground ? s.jump_power : s.double_jump_power);
                        --p.jumps_left;
                        p.on_ground = false;
                        p.support = -1;
                        p.state = ST_AIR;
                        p.down_hold = 0;
                        ev.jump = true;
                    }
                    else if(in.down && p.on_ground && p.support >= 0 && p.state != ST_SHIELD)
                    {
                        if(++p.down_hold >= DROP_HOLD)                    // DOWN has to be HELD: a quick DOWN + A/B is a down move
                        {
                            p.down_hold = 0;
                            p.drop_timer = DROP_FRAMES;                   // fall through the platform
                            p.on_ground = false;
                            p.support = -1;
                            p.state = ST_AIR;
                        }
                    }
                    else { p.down_hold = 0; }

                    const bool can_guard = p.on_ground && !p.shield_locked && p.shield > 0;
                    if(in.shield && can_guard)            { p.state = ST_SHIELD; }
                    else if(p.state == ST_SHIELD)         { p.state = ST_IDLE; }
                }
                steering = p.state != ST_SHIELD && p.state != ST_ATTACK;
            }
            else { p.down_hold = 0; }

            // ---- the shield's energy: it drains while held, so its defense boost slowly drops ----
            if(p.state == ST_SHIELD)
            {
                p.shield -= SHIELD_DRAIN;
                if(p.shield <= 0)
                {
                    p.shield = 0;
                    p.shield_locked = true;                               // broken: no guarding until it recharges
                    p.state = ST_IDLE;
                }
            }
            else
            {
                p.shield += SHIELD_REGEN;
                if(p.shield > SHIELD_MAX) { p.shield = SHIELD_MAX; }
                if(p.shield_locked && p.shield >= SHIELD_REACTIVATE) { p.shield_locked = false; }
            }

            // ---- sideways speed ----
            if(steering)
            {
                const int target = in.move * s.walk_speed;
                int accel = p.on_ground ? s.ground_accel : s.air_accel;
                if(target == 0 && p.on_ground) { accel = s.friction; }
                p.vx = approach(p.vx, target, accel);
            }
            else if(p.on_ground && !floating)
            {
                p.vx = approach(p.vx, 0, s.friction); // shielding, attacking, flinching: slide to a stop
            }

            integrate(p, in.down, floating);

            // ---- a dive that touched the ground: boom, and only the recovery is left ----
            if(p.state == ST_ATTACK && p.move && p.driving && p.on_ground && (p.move->flags & MF_FIRE_ON_LAND) && !p.land_fired)
            {
                p.land_fired = true;
                p.drive_cut = true;
                p.vx = 0;
                spawn_shot(i, p.move->shot);
                ev.boom = true;
                if(p.move_frame < p.move->land_frame) { p.move_frame = p.move->land_frame; }
            }

            // ---- a downward dash (dair) that touched the ground ends: only the landing lag is left ----
            if(p.state == ST_ATTACK && p.move && p.driving && p.on_ground && p.move->land_end > 0 && !p.land_fired)
            {
                p.land_fired = true;
                p.drive_cut = true;
                p.vx = 0;
                if(p.move_frame < p.move->land_end) { p.move_frame = p.move->land_end; }
            }

            // ---- which of idle / walk / air are we? ----
            if(p.state == ST_IDLE || p.state == ST_WALK || p.state == ST_AIR)
            {
                if(!p.on_ground)                              { p.state = ST_AIR; }
                else if(in.move != 0 && p.vx != 0)            { p.state = ST_WALK; }
                else                                          { p.state = ST_IDLE; }
            }
        }

        // The buttons asked for a move. Returns false if this fighter has no move there (or can't use it now).
        bool try_start_move(Fighter& p, const Input& in)
        {
            const FighterStats& s = *p.stats;
            if(!s.moves) { return false; }

            const int slot = pick_slot(in, p);
            const Move* m = s.moves->slot[slot];
            if(!m) { return false; }
            if(m->alt && p.has_stored) { m = m->alt; }                    // holding a stolen shot: fox thief becomes the throw
            if((m->flags & MF_AIR_ONCE) && !p.on_ground && (p.air_used & (1u << slot))) { return false; }

            // you turn toward the direction you press, except for aerials: those are "forward" and "back" of where you face
            const bool aerial = in.attack && !in.special && !p.on_ground;
            if(in.move != 0 && !aerial) { p.facing = in.move; }

            begin_move(p, m, in.special);                                 // B = magic, A = physical (for the damage buffs)
            if(!p.on_ground && (m->flags & MF_AIR_ONCE)) { p.air_used |= 1u << slot; }
            if(m->hit_count > 0 || m->drive.end > m->drive.start) { ev.swing = true; }
            return true;
        }

        // Starts a move from frame 0 (also used when one move hands over to another: counter, grab...)
        static void begin_move(Fighter& p, const Move* m, bool magic)
        {
            p.state = ST_ATTACK;
            p.move = m;
            p.move_frame = 0;
            p.hit_done = 0;
            p.driving = false;
            p.drive_cut = false;
            p.land_fired = false;
            p.move_magic = magic;
            p.down_hold = 0;
        }

        // Runs frame `move_frame` of the fighter's move: flying and firing. Returns true while gravity must stay off.
        bool run_move(int i)
        {
            Fighter& p = f[i];
            const Move& m = *p.move;
            const int t = p.move_frame;
            bool floating = false;

            const Drive& d = m.drive;
            if(d.end > d.start && t >= d.start && t < d.end && !p.drive_cut)
            {
                p.vx = p.facing * d.vx;
                p.vy = d.vy;

                if(d.homing > 0)                                          // steer up or down toward the opponent (if they are ahead)
                {
                    const Fighter& op = f[foe_of(i)];
                    if((op.x16 - p.x16) * p.facing > 0)
                    {
                        const int want = (op.y16 - (op.stats->body_h * FP) / 2) - (p.y16 - (p.stats->body_h * FP) / 2);
                        const int limit = abs_i(d.vx) * d.homing / 100;
                        p.vy = want > limit ? limit : (want < -limit ? -limit : want);
                    }
                }

                if(t == d.start) { ev.dash = true; }
                p.driving = true;
                floating = true;
            }
            else if(p.driving)                                            // the dash just ended: throw most of its speed away
            {
                p.driving = false;
                p.vx = p.vx * DRIVE_EXIT_PERCENT / 100;
                p.vy = p.vy * DRIVE_EXIT_PERCENT / 100;
            }

            if(m.leap_frame >= 0 && t == m.leap_frame)                    // thrown off the ground like a jump (gravity does the rest)
            {
                p.vx = p.facing * m.leap_vx;
                p.vy = m.leap_vy;
                p.on_ground = false;
                p.support = -1;
            }

            if(m.shot_frame >= 0 && t == m.shot_frame)
            {
                if(m.flags & MF_USE_STORED) { if(p.has_stored) { throw_stored(i, m.power); } }
                else if(m.shot_list && m.shot_list_count > 0) { spawn_shot(i, m.shot_list[random_below(m.shot_list_count)]); ev.shot = true; }
                else                        { spawn_shot(i, m.shot); ev.shot = true; }
            }
            return floating;
        }

        void advance_move(int i)
        {
            Fighter& p = f[i];
            if(p.state != ST_ATTACK || !p.move) { return; }
            if(++p.move_frame >= p.move->duration)
            {
                                if(p.driving) { p.vx = p.vx * DRIVE_EXIT_PERCENT / 100; p.vy = p.vy * DRIVE_EXIT_PERCENT / 100; }
                if(p.move->flags & MF_ORBIT_HELD) { p.has_stored = false; }
                p.driving = false;
                p.move = nullptr;
                p.state = p.on_ground ? ST_IDLE : ST_AIR;
            }
        }

        // gravity, movement, walls, and landing on the floor or a platform
        void integrate(Fighter& p, bool holding_down, bool floating)
        {
            const FighterStats& s = *p.stats;

            if(!floating)                                                 // (an aqua jet ignores gravity)
            {
                p.vy += s.gravity;
                const int limit = (holding_down && p.state != ST_KO) ? s.fast_fall : s.max_fall;
                if(p.vy > limit) { p.vy = limit; }
            }

            const int old_y16 = p.y16;
            p.x16 += p.vx;
            p.y16 += p.vy;

            const int min_x = BODY_MARGIN * FP;
            const int max_x = (stage.length - BODY_MARGIN) * FP;
            if(p.x16 < min_x) { p.x16 = min_x; p.vx = 0; }
            if(p.x16 > max_x) { p.x16 = max_x; p.vx = 0; }

            bool landed = false;
            int support = -1;
            if(p.vy >= 0)
            {
                int best_y16 = stage.ground_y * FP;
                if(p.y16 >= best_y16) { landed = true; }

                if(p.drop_timer == 0)
                {
                    const int foot_x = p.x16 / FP;
                    for(int k = 0; k < stage.platform_count; ++k)
                    {
                        const Platform& pl = stage.platforms[k];
                        const int top16 = pl.y * FP;
                        if(foot_x >= pl.x && foot_x <= pl.x + pl.w && old_y16 <= top16 && p.y16 >= top16 && (!landed || top16 < best_y16))
                        {
                            landed = true;
                            best_y16 = top16;
                            support = k;
                        }
                    }
                }

                if(landed)
                {
                    p.y16 = best_y16;
                    p.vy = 0;
                }
            }

            p.on_ground = landed;
            p.support = landed ? support : -1;
            if(landed) { p.jumps_left = s.max_jumps; p.air_used = 0; }
        }

        // ---------------------------------------------------------------------------- shots

        // Puts a shot into the world, in front of the fighter who fires it.
        // (the owner's damage buff is applied here, except for a shot that was stolen: that one was buffed already)
        void spawn_shot(int owner, const Shot& shot, bool apply_buff = true)
        {
            Fighter& p = f[owner];
            spawn_shot_at(owner, shot, p.x16 + p.facing * shot.dx * FP, p.y16 - shot.dy * FP, p.facing * shot.speed, apply_buff);
        }

        void spawn_shot_at(int owner, const Shot& shot, int x16, int y16, int vx, bool apply_buff = true)
        {
            Fighter& p = f[owner];
            Shot sh = shot;
            if(apply_buff) { sh.damage = buffed_damage(p, sh.damage); }
            for(Projectile& pr : proj)
            {
                if(pr.active) { continue; }
                pr = Projectile();
                pr.active = true;
                pr.id = next_projectile_id++;
                pr.owner = owner;
                pr.x16 = x16;
                pr.y16 = y16;
                pr.vx = vx;
                pr.vy = sh.vy;
                pr.life = sh.life;
                pr.shot = sh;
                return;
            }
        }

        // a laptop (or anything with boom > 0) blows up where it hit
        void spawn_boom(int owner, int x16, int y16, int damage)
        {
            const Shot boom = Shot().of(PROJ_EXPLOSION).dmg(damage).fly(0, 10).size(40, 30)
                                    .launch(fp(3.5), fp(-4.0)).stun(22).steal_proof().lingers();
            spawn_shot_at(owner, boom, x16, y16, 0);
            ev.boom = true;
        }

        // fox thief, second press: send the stolen shot back (a bit stronger and faster)
        void throw_stored(int i, int power)
        {
            Fighter& p = f[i];
            Shot sh = p.stored;
            sh.speed = sh.speed * power / 100;
            sh.damage = sh.damage * power / 100;
            if(sh.damage < 1) { sh.damage = 1; }
            spawn_shot(i, sh, false);
            p.has_stored = false;
            ev.shot = true;
        }

        void update_projectiles()
        {
            
            for(Projectile& pr : proj)
            {
                if(!pr.active) { continue; }

                pr.x16 += pr.vx;
                if(pr.shot.grav != 0 || pr.vy != 0)                       // thrown objects fly in an arc
                {
                    pr.vy += pr.shot.grav;
                    pr.y16 += pr.vy;
                }
                ++pr.age;
                if(--pr.life <= 0 || pr.x16 < -32 * FP || pr.x16 > (stage.length + 32) * FP) { pr.active = false; continue; }

                if(pr.shot.grav != 0 && pr.vy > 0 && pr.y16 >= stage.ground_y * FP)   // an arc that came down onto the floor
                {
                    pr.active = false;
                    if(pr.shot.boom > 0) { spawn_boom(pr.owner, pr.x16, stage.ground_y * FP - 8 * FP, pr.shot.boom); }
                    continue;
                }

                const Shot& sh = pr.shot;
                const Rect box{ pr.x16 / FP, pr.y16 / FP, sh.w, sh.h };

                // a shot hits the first fighter of the OTHER team that it touches (never its own side)
                for(int ti = 0; ti < count && pr.active && !pr.hit_done; ++ti)
                {
                    if(team_of(ti) == team_of(pr.owner) || f[ti].state == ST_KO) { continue; }

                    Fighter& target = f[ti];
                    Rect body = hurtbox_of(target);

                    // fox thief: while the stance is up, a shot that comes close is stolen instead of hurting
                    const Move* tm = target.move;
                    if(sh.absorbable && !target.has_stored && target.state == ST_ATTACK && tm && tm->absorb_end > tm->absorb_start &&
                       target.move_frame >= tm->absorb_start && target.move_frame < tm->absorb_end)
                    {
                        Rect catch_zone = body;
                        catch_zone.w += ABSORB_MARGIN * 2;
                        catch_zone.h += ABSORB_MARGIN * 2;
                        if(overlaps(box, catch_zone))
                        {
                            target.has_stored = true;
                            target.stored = sh;
                            pr.active = false;
                            ev.absorb = true;
                            if(tm->on_absorb) { begin_move(target, tm->on_absorb, target.move_magic); }   // (Filip: spin and throw it back at once)
                            break;
                        }
                    }

                    if(overlaps(box, body))
                    {
                        int dir = sign_i(pr.vx);
                        if(dir == 0) { dir = sign_i(target.x16 - pr.x16); }
                        if(dir == 0) { dir = f[pr.owner].facing; }
                        hit(target, sh.damage, sh.kb_x, sh.kb_y, sh.hitstun, dir, pr.x16, pr.y16, &f[pr.owner]);
                        if(sh.boom > 0) { spawn_boom(pr.owner, pr.x16, pr.y16, sh.boom); }
                        if(sh.persist) { pr.hit_done = true; } else { pr.active = false; }
                    }
                }
            }
        }

        // ---------------------------------------------------------------------------- melee

        // Checks every active hitbox against the other team. Everything that touches this frame is collected first and
        // applied afterwards, so two fighters that hit each other on the same frame BOTH get hit (a trade).
        void resolve_melee()
        {
            PendingHit pending[3 * MAX_HITBOXES * 2];
            int pending_count = 0;

            for(int a = 0; a < count; ++a)
            {
                const Fighter& p = f[a];
                if(p.state != ST_ATTACK || !p.move) { continue; }

                for(int v = 0; v < count; ++v)
                {
                    const Fighter& t = f[v];
                    if(team_of(v) == team_of(a) || t.state == ST_KO) { continue; }

                    const Rect body = hurtbox_of(t);
                    for(int k = 0; k < p.move->hit_count; ++k)
                    {
                        Rect box;
                        if(!hitbox_now(a, k, box) || (p.hit_done & (1u << k)) || !overlaps(box, body)) { continue; }

                        const Hitbox& h = p.move->hits[k];
                        const int gap = t.x16 - p.x16;
                        PendingHit& ph = pending[pending_count++];
                        ph.attacker = a;
                        ph.victim = v;
                        ph.index = k;
                        ph.box = h;
                        // launched away from the attacker (so a hit BEHIND flies backwards). Straight above or below: use the hitbox's side
                        ph.dir = abs_i(gap) < 3 * FP ? (h.absolute ? (h.x < 0 ? -1 : 1) : (h.x < 0 ? -p.facing : p.facing)) : sign_i(gap);
                        ph.sx16 = ((box.x + body.x) / 2) * FP;
                        ph.sy16 = ((box.y + body.y) / 2) * FP;
                        ph.stops_dash = p.move->drive.stop_on_hit;
                    }
                }
            }

            for(int n = 0; n < pending_count; ++n)
            {
                const PendingHit& ph = pending[n];
                Fighter& attacker = f[ph.attacker];
                Fighter& victim = f[ph.victim];
                attacker.hit_done |= 1u << ph.index;
                if(attacker.state == ST_GRABBED) { continue; }                 // (it was just caught by a counter: its other hits are void)
                if(ph.stops_dash && attacker.state == ST_ATTACK) { attacker.drive_cut = true; }

                // a grab: no damage, the grabber's next move takes over (Filip's leap)
                if(ph.box.grab) { grab_connects(ph.attacker, ph.victim); continue; }

                // Filip's counter: a melee hit that lands inside the counter window is caught and answered
                const int fs_id = (attacker.move && attacker.move->final_smash != FS_NONE) ? attacker.move->final_smash : FS_NONE;

                const Move* vm = victim.move;
                if(fs_id == FS_NONE && vm && vm->on_counter_melee && in_window(victim, vm->counter_start, vm->counter_end))
                {
                    counter_melee(ph.victim, ph.attacker);
                    continue;
                }

                const int damage = buffed_damage(attacker, ph.box.damage);

                if(ph.box.keep_grab && victim.grabbed_by == ph.attacker)
                {
                    scratch(victim, damage, ph.box.kb_x, ph.box.kb_y, ph.dir, fs_id != FS_NONE ? nullptr : &attacker);
                    spawn_effect(FX_SPARK, ph.sx16, ph.sy16, 10);
                    continue;
                }

                int applied_dmg = damage;
                if(fs_id != FS_NONE)
                {
                    if(victim.state == ST_SHIELD) { victim.state = ST_IDLE; }
                    if(victim.hp <= applied_dmg) { applied_dmg = victim.hp > 1 ? victim.hp - 1 : 0; }
                }

                hit(victim, applied_dmg, ph.box.kb_x, ph.box.kb_y, ph.box.hitstun, ph.dir, ph.sx16, ph.sy16, fs_id != FS_NONE ? nullptr : &attacker);
                if(fs_id != FS_NONE && pending_final_smash < 0)
                {
                    pending_final_smash = ph.attacker;
                    pending_final_smash_victim = ph.victim;
                    pending_final_smash_id = fs_id;
                }
                if(ph.box.pogo > 0) { pogo(attacker, ph.box.pogo); }
            }
        }

        // ---------------------------------------------------------------------------- grabs, counters, pogo

        static bool in_window(const Fighter& t, int from, int to)
        {
            return t.state == ST_ATTACK && t.move && to > from && t.move_frame >= from && t.move_frame < to;
        }

        // `victim` is now held by `grabber`: it can't act and is kept in front of the grabber until it is hit or let go
        void begin_grab(int grabber, int victim)
        {
            Fighter& g = f[grabber];
            Fighter& v = f[victim];
            cancel_move(v);
            v.state = ST_GRABBED;
            v.grabbed_by = grabber;
            v.vx = 0;
            v.vy = 0;
            v.timer = 0;
            g.holding = victim;
            g.vx = 0;
            g.vy = 0;
        }

        void release_grab(int victim)
        {
            Fighter& v = f[victim];
            if(v.grabbed_by >= 0)
            {
                Fighter& g = f[v.grabbed_by];
                if(g.holding == victim) { g.holding = -1; }
                v.grabbed_by = -1;
            }
            if(v.state == ST_GRABBED) { v.state = ST_HURT; v.timer = GRAB_RELEASE_STUN; }
        }

        void clear_grab_of(Fighter& t)                                // (the victim got hit for real: it is not held any more)
        {
            if(t.grabbed_by < 0) { return; }
            Fighter& g = f[t.grabbed_by];
            if(g.holding == &t - f) { g.holding = -1; }
            t.grabbed_by = -1;
        }

        // keeps every grabbed fighter in front of the one holding it. If the grabber is interrupted (hit, or its move ended)
        // the victim is let go.
        void update_grabs()
        {
            for(int v = 0; v < count; ++v)
            {
                Fighter& vf = f[v];
                if(vf.grabbed_by < 0) { continue; }
                Fighter& gf = f[vf.grabbed_by];
                if(winner >= 0 || gf.state != ST_ATTACK || gf.holding != v || vf.state == ST_KO) { release_grab(v); continue; }

                int x16 = gf.x16 + gf.facing * GRAB_DISTANCE * FP;
                if(x16 < BODY_MARGIN * FP) { x16 = BODY_MARGIN * FP; }
                if(x16 > (stage.length - BODY_MARGIN) * FP) { x16 = (stage.length - BODY_MARGIN) * FP; }
                vf.x16 = x16;
                vf.y16 = gf.y16;
                vf.vx = 0;
                vf.vy = 0;
                vf.facing = -gf.facing;
                vf.on_ground = gf.on_ground;
                vf.support = gf.support;
            }
        }

        // a hitbox with grabs() touched somebody (shields don't stop a grab)
        void grab_connects(int grabber, int victim)
        {
            Fighter& g = f[grabber];
            const Fighter& v = f[victim];
            if(v.state == ST_KO || v.grabbed_by >= 0 || g.holding >= 0) { return; }
            const Move* next = g.move ? g.move->on_grab : nullptr;
            begin_grab(grabber, victim);
            if(next) { begin_move(g, next, g.move_magic); }
            spawn_effect(FX_SPARK, v.x16, v.y16 - (v.stats->body_h * FP) / 2, 10);
            ev.hit = true;
        }

        // Filip's counter caught a melee hit: the attacker is grabbed, and the counter's next move (grab + punch) starts
        void counter_melee(int counter, int attacker)
        {
            Fighter& c = f[counter];
            const Fighter& a = f[attacker];
            const Move* next = c.move->on_counter_melee;
            const int dir = sign_i(a.x16 - c.x16);
            if(dir != 0) { c.facing = dir; }
            begin_grab(counter, attacker);
            begin_move(c, next, c.move_magic);
            spawn_effect(FX_SPARK, (a.x16 + c.x16) / 2, c.y16 - (c.stats->body_h * FP) / 2, 10);
            ev.absorb = true;
        }

        // damage to somebody who stays held (the three scratches). If it would knock them out, it is a normal hit.
        void scratch(Fighter& t, int damage, int kb_x, int kb_y, int dir, Fighter* attacker = nullptr)
        {
            int dealt = damage * (100 - t.stats->defense) / 100;
            if(dealt < 1) { dealt = 1; }
            if(t.hp - dealt <= 0) { hurt(t, damage, kb_x, kb_y, 0, dir, attacker); return; }
            t.hp -= dealt;
            if(attacker && attacker != &t && attacker->stats->final_smash_move != nullptr && attacker->smash < SMASH_MAX)
            {
                attacker->smash += dealt * SMASH_HIT_GAIN;
                if(attacker->smash > SMASH_MAX) { attacker->smash = SMASH_MAX; }
            }
            if(t.stats->final_smash_move != nullptr && t.smash < SMASH_MAX)
            {
                t.smash += dealt * SMASH_HURT_GAIN;
                if(t.smash > SMASH_MAX) { t.smash = SMASH_MAX; }
            }
            ev.hit = true;
        }

        // the attacker bounced off the victim (pogo): thrown upward, the dash is over, only a short recovery is left
        static void pogo(Fighter& a, int speed)
        {
            a.vy = -speed;
            a.vx = a.vx / 2;
            a.on_ground = false;
            a.support = -1;
            a.driving = false;
            a.drive_cut = true;
            if(a.state == ST_ATTACK && a.move && a.move_frame < a.move->duration - POGO_RECOVERY)
            {
                a.move_frame = a.move->duration - POGO_RECOVERY;
            }
        }

        // ---------------------------------------------------------------------------- damage buffs

        // The attacker's damage buff (phys_pct for A moves, magic_pct for B moves). Whole numbers lose the small parts
        // (+20% of a 2 damage slap is 0.4), so what is missing is kept in dmg_carry and paid out on later hits:
        // five slaps of 2 really do add up to 12 instead of 10.
        static int buffed_damage(Fighter& attacker, int damage)
        {
            if(damage <= 0) { return damage; }                            // (a hit that only stuns stays a hit that only stuns)
            const int type = attacker.move_magic ? 1 : 0;
            const int pct = type == 1 ? attacker.stats->magic_pct : attacker.stats->phys_pct;
            if(pct == 100) { return damage; }

            attacker.dmg_carry[type] += damage * (pct - 100);
            const int extra = attacker.dmg_carry[type] / 100;
            attacker.dmg_carry[type] -= extra * 100;
            const int result = damage + extra;
            return result < 1 ? 1 : result;
        }

        // ---------------------------------------------------------------------------- getting hit

        void spawn_effect(int kind, int x16, int y16, int life)
        {
            for(Effect& e : fx)
            {
                if(e.active) { continue; }
                e.active = true;
                e.kind = kind;
                e.x16 = x16;
                e.y16 = y16;
                e.age = 0;
                e.life = life;
                return;
            }
        }

        void hit(Fighter& t, int damage, int kb_x, int kb_y, int stun, int dir, int sx16, int sy16, Fighter* attacker = nullptr)
        {
            hurt(t, damage, kb_x, kb_y, stun, dir, attacker);
            spawn_effect(FX_SPARK, sx16, sy16, 10);
        }

        // KNOCKBACK: the victim is thrown with the hit's launch speed. The less HP is left, the further they fly,
        // and heavier fighters fly less. dir = +1 launches to the right.
        void launch(Fighter& t, int kb_x, int kb_y, int dir)
        {
            const FighterStats& s = *t.stats;
            int scale = 100 + KB_HP_BONUS * (s.max_hp - t.hp) / s.max_hp;
            scale = scale * 100 / (s.weight > 0 ? s.weight : 100);
            t.vx = dir * kb_x * scale / 100;
            t.vy = kb_y * scale / 100;
            if(t.vy < 0) { t.on_ground = false; t.support = -1; }         // thrown upward: you leave the ground
        }

        // Damage goes through the defenses. A shield removes a LOT of it, but the boost fades as the shield drains.
        void hurt(Fighter& t, int damage, int kb_x, int kb_y, int stun, int dir, Fighter* attacker = nullptr)
        {
            const FighterStats& s = *t.stats;
            clear_grab_of(t);
            int reduction = s.defense;
            const bool guarding = t.state == ST_SHIELD;
            if(guarding) { reduction += SHIELD_MAX_REDUCTION * t.shield / SHIELD_MAX; }
            if(reduction > 95) { reduction = 95; }

            int dealt = damage * (100 - reduction) / 100;
            if(dealt < 1 && damage > 0) { dealt = 1; }                    // there is always a little chip damage (a 0 damage hit only stuns)

            if(attacker && attacker != &t && attacker->stats->final_smash_move != nullptr && attacker->smash < SMASH_MAX)
            {
                attacker->smash += dealt * SMASH_HIT_GAIN;
                if(attacker->smash > SMASH_MAX) { attacker->smash = SMASH_MAX; }
            }
            if(t.hp - dealt > 0 && t.stats->final_smash_move != nullptr && t.smash < SMASH_MAX)
            {
                t.smash += dealt * SMASH_HURT_GAIN;
                if(t.smash > SMASH_MAX) { t.smash = SMASH_MAX; }
            }

            // super armor: a hit that is not strong enough neither flinches nor launches. A strong one breaks through.
            const Move* tm = t.move;
            if(tm && tm->armor_end > tm->armor_start && in_window(t, tm->armor_start, tm->armor_end) &&
               !guarding && dealt <= tm->armor_limit && t.hp - dealt > 0)
            {
                t.hp -= dealt;
                ev.hit = true;
                return;
            }

            t.hp -= dealt;

            if(t.hp <= 0)
            {
                t.hp = 0;
                cancel_move(t);
                t.has_stored = false;
                t.state = ST_KO;
                t.timer = 0;
                launch(t, kb_x, kb_y, dir);                               // the final blow sends them flying too
                ev.ko = true;
            }
            else if(guarding)
            {
                ev.blocked = true;
                t.vx = dir * abs_i(kb_x) / SHIELD_PUSH;                   // a guard is only pushed back a little
                t.shield -= dealt * SHIELD_HIT_COST;                      // taking hits wears the shield down faster
                if(t.shield <= 0) { t.shield = 0; t.shield_locked = true; t.state = ST_IDLE; }
            }
            else
            {
                if (dealt >= 10){ ev.hitsmash = true;} else {ev.hit = true;}
                if (dealt >= 3 || damage == 0) {cancel_move(t);}                            // being hit interrupts whatever you were doing
                t.state = ST_HURT;
                t.timer = stun > 0 ? stun : HURT_FRAMES;
                launch(t, kb_x, kb_y, dir);
            }
        }

        static void cancel_move(Fighter& t)
        {
            if(t.has_stored &&
               ((t.move && (t.move->flags & MF_ORBIT_HELD)) ||
                !t.stats || !t.stats->moves || !t.stats->moves->slot[SLOT_SPECIAL_DOWN] || !t.stats->moves->slot[SLOT_SPECIAL_DOWN]->alt))
            {
                t.has_stored = false;
            }
            t.move = nullptr;
            t.move_frame = 0;
            t.driving = false;
            t.drive_cut = false;
        }
    };
}

#endif
