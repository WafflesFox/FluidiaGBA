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
    constexpr int FS_FREEZE_FRAMES = 42;        // dramatic freeze frames when a Final Smash starts
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
    constexpr int MAX_FIGHTERS = 5;
    constexpr int CLONE_CHARGE_MAX = 75;
    constexpr int CLONE_HP_COST = 5;
    constexpr int CLONE_LIFE_FRAMES = 60 * 20;
    constexpr int MEAL_HEAL = 50;
    constexpr int MEAL_DEF_BONUS = 35;
    constexpr int MEAL_DEF_FRAMES = 600;
    constexpr int HAR_CHARGE_MAX = 90;
    constexpr int HAR_POWER_FRAMES = 60 * 20;
    constexpr int HAR_BOMB_DELAY = 120;
    constexpr int MAX_PLATFORMS = 6;
    constexpr int MAX_HITBOXES = 4;             // per move (raise it if a move needs more)
    constexpr int MAX_EFFECTS = 6;              // hit sparks & dust puffs on screen at once
    constexpr int PARTNER_SPACING = 24;         // a partner starts this many pixels behind the player
    constexpr int GRAB_DISTANCE = 16;           // a grabbed fighter is held this many pixels in front of the one who grabbed
    constexpr int GRAB_RELEASE_STUN = 14;       // a grabbed fighter that is let go without a hit (the grabber was interrupted)
    constexpr int POGO_RECOVERY = 10;           // frames of a move that are left after it bounced off somebody (pogo)

    // ---- Umbreon / Minecraft mechanics ----
    constexpr int MAX_BLOCKS = 4;               // max air-placed blocks active at once
    constexpr int MAX_TNT = 3;                  // max active TNT traps at once
    constexpr int BLOCK_W = 16;
    constexpr int BLOCK_H = 16;
    constexpr int BLOCK_LIFE = 240;             // 4 seconds
    constexpr int BLOCK_HP = 16;
    constexpr int TNT_ARM_FRAMES = 20;          // grace delay before TNT triggers on touch
    constexpr int TNT_LIFE = 600;
    constexpr int PIG_MAX_HP = 36;
    constexpr int PIG_HIT_CD = 28;
    constexpr int PIG_MAX_HITS = 3;
    constexpr int MAX_MATERIALS = 64;
    constexpr int MINE_GAIN = 5;
    constexpr int MAX_WEAPON_TIER = 4;          // 1=Wood(3), 2=Stone(10), 3=Iron(30), 4=Diamond(50)

    // ============================================================================================
    // DATA: what a move is, what a fighter is, what a stage is
    // ============================================================================================

    enum
    {
        PROJ_WATER = 0, PROJ_EXPLOSION,
        PROJ_JUNK_PENCIL, PROJ_JUNK_APPLE, PROJ_JUNK_BAT, PROJ_JUNK_LAPTOP, PROJ_JUNK_BALL,
        PROJ_FIREBALL, PROJ_BOOMERANG, PROJ_ARROW, PROJ_MINECART, PROJ_TNT, PROJ_SONIC_SLASH
    };
    enum { FX_SPARK = 0, FX_DUST = 1 };
    enum FinalSmashId { FS_NONE = 0, FS_WAFFLE_FIRE, FS_FILIP_DROP, FS_UMB_MINECART, FS_WAFFLE_MEAL , FS_HARUE_COMBO , FS_HARUE_SLICE };

    // Which pose to show during a move. You have no attack sprites yet, so battle.h shows stand-in frames.
    // When you draw real ones, only pick_frame() in battle.h needs to change.
    enum
    {
        // Waffles
        ANIM_STAND, ANIM_SLAP, ANIM_TAIL, ANIM_TAIL_UP, ANIM_KICK, ANIM_SPIN, ANIM_SLAM,
        ANIM_SHOOT, ANIM_JET_SIDE, ANIM_JET_UP, ANIM_JET_DOWN, ANIM_STANCE, 
        ANIM_WAF_CHARGE, ANIM_WAF_STEAL,
        // Filip and Umbreon
        ANIM_SNEP_JAB, ANIM_SNEP_HEAVY, ANIM_SNEP_FLIP, ANIM_SNEP_DSMASH, ANIM_SNEP_COUNTER, ANIM_SNEP_COUNTER_GRAB,
        ANIM_SNEP_SPIN_THROW, ANIM_SNEP_THROW_UP, ANIM_SNEP_LEAP, ANIM_SNEP_SCRATCH, ANIM_SNEP_DAIR,
        ANIM_UMB_MINE, ANIM_UMB_SLASH, ANIM_UMB_PLACE, ANIM_UMB_BOW, ANIM_UMB_THROW, ANIM_RTS, ANIM_SNEP_PUNCH,
        
        // Harue (dad)
        ANIM_HAR_JAB, ANIM_HAR_FSMASH, ANIM_HAR_USMASH, ANIM_HAR_SPIN,
        ANIM_HAR_CHARGE, ANIM_HAR_BSHOT, ANIM_HAR_DASH, ANIM_HAR_UPDOWN,
        ANIM_HAR_REFLECT, ANIM_HAR_FS,
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
        bool weapon_scaled = false;
        bool hits_ally = false;
        int delayed_boom_dmg = 0;

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
        constexpr Hitbox weapon() const             { Hitbox b = *this; b.weapon_scaled = true; return b; }
        constexpr Hitbox hits_teammates() const     { Hitbox b = *this; b.hits_ally = true; return b; }
        constexpr Hitbox delayed_boom(int dmg) const { Hitbox b = *this; b.delayed_boom_dmg = dmg; return b; }
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
        int spread = 0;
        int boom_accel = 0;
        bool weapon_scaled = false;
        bool friendly_fire = false;
        int heal_ally = 0;
        int final_smash = FS_NONE;

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
        constexpr Shot fan(int spread_vx_fp) const     { Shot s = *this; s.spread = spread_vx_fp; return s; }
        constexpr Shot boomerang(int accel_fp) const   { Shot s = *this; s.boom_accel = accel_fp; return s; }
        constexpr Shot weapon() const                  { Shot s = *this; s.weapon_scaled = true; return s; }
        constexpr Shot hurts_everyone() const          { Shot s = *this; s.friendly_fire = true; return s; }
        constexpr Shot starts_final_smash(int id) const { Shot s = *this; s.final_smash = id; return s; }
        constexpr Shot heals_ally(int hp) const         { Shot s = *this; s.heal_ally = hp; return s; }
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
        Drive drive2 = {};
        int shot_frame = -1;        // the frame the shot is fired (-1 = this move fires nothing at a set time)
        Shot shot = {};
        int land_frame = 0;         // MF_FIRE_ON_LAND: the move jumps to this frame when it lands (so only the recovery is left)
        int absorb_start = 0, absorb_end = 0;   // frames where enemy shots that touch the fighter are stolen
        int reflect_start = 0, reflect_end = 0;
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
        int final_smash = FS_NONE;
        int tech_frame = -1;
        bool mines = false;
        bool crafts = false;
        bool places_block = false;
        bool places_tnt = false;
        bool mounts_pig = false;
        bool charges_clone = false;
        int ally_final_smash = FS_NONE;
        bool charges_power = false;
        bool summons_har_clone = false;
        bool in_stage_fs = false;
        int powered_fs = FS_NONE;

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
        constexpr Move mine_mat(int frame) const     { Move m = *this; m.tech_frame = frame; m.mines = true; return m; }
        constexpr Move craft_weapon(int frame) const { Move m = *this; m.tech_frame = frame; m.crafts = true; return m; }
        constexpr Move place_block(int frame) const  { Move m = *this; m.tech_frame = frame; m.places_block = true; return m; }
        constexpr Move place_tnt(int frame) const    { Move m = *this; m.tech_frame = frame; m.places_tnt = true; return m; }
        constexpr Move mount_pig(int frame) const    { Move m = *this; m.tech_frame = frame; m.mounts_pig = true; return m; }
        constexpr Move charge_clone() const          { Move m = *this; m.charges_clone = true; return m; }
        constexpr Move ally_fs(int id) const         { Move m = *this; m.ally_final_smash = id; return m; }
        constexpr Move dash2(const Drive& d) const { Move m = *this; m.drive2 = d; return m; }
        constexpr Move reflects(int s, int e) const { Move m = *this; m.reflect_start = s; m.reflect_end = e; return m; }
        constexpr Move charge_power() const { Move m = *this; m.charges_power = true; return m; }
        constexpr Move summon_har_clone(int frame = 6) const { Move m = *this; m.tech_frame = frame; m.summons_har_clone = true; return m; }
        constexpr Move in_stage_final_smash(int id) const { Move m = *this; m.final_smash = id; m.in_stage_fs = true; return m; }
        constexpr Move powered_final_smash(int id) const { Move m = *this; m.powered_fs = id; return m; }
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
        bool spawns_crafting_table = false;
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
        bool special_held = false; // B held this frame
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
        bool has_table = false;
        int table_x16 = 0, table_y16 = 0;
        int materials = 0;
        int weapon_tier = 0;
        int weapon_dur = 0;
        bool on_pig = false;
        int pig_hp = 0;
        int pig_hit_cd = 0;
        int pig_hits = 0;
        bool active = false;
        bool is_clone = false;
        int clone_of = -1;
        bool shadow_clone = false;      // true = Harue's Shadow Twin (drawn purple). false = Waffles' Water Clone (cyan)
        int clone_charge = 0;
        int clone_timer = 0;
        int def_buff_timer = 0;
        int def_buff_amount = 0;
        int har_charge = 0;
        int har_power_timer = 0;
        int bomb_timer = 0;
        int bomb_owner = -1;
        int bomb_dmg = 0;
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
        int boom_dir = 1;
        bool boom_flipped = false;
        Shot shot = {};
    };

    struct PlacedBlock
    {
        bool active = false;
        int id = 0;
        int owner = 0;
        int x = 0, y = 0, w = BLOCK_W;
        int life = 0;
        int hp = BLOCK_HP;
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
        bool final_smash_start = false;
        bool grab = false;
        bool mining = false;
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
        int opp_proj_count = 0;
        int opp_rush_count = 0;
        int opp_shield_count = 0;
        int opp_air_count = 0;
        int adapt_timer = 0;
        int charge_hold = 0;
        int last_opp_state = 0;
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
        Fighter f[MAX_FIGHTERS];               // f[0] = the player, f[1] = the opponent, f[2] = the player's partner (only if count == 3)
        int count = 2;              // 2 = one on one, 3 = you and a partner against the opponent
        Projectile proj[MAX_PROJECTILES];
        PlacedBlock blocks[MAX_BLOCKS];
        Effect fx[MAX_EFFECTS];
        Ai ai[MAX_FIGHTERS];
        FighterStats clone_stats[2] = {};
        const FighterStats* waffle_ref_stats = nullptr;
        Events ev;
        unsigned rng = 1;
        int frame = 0;
        int winner = -1;            // -1 = still fighting, 0 = the player won, 1 = the opponent won
        int pending_final_smash = -1;
        int fs_slow_tick = 0;
        int fs_freeze_timer = 0;
        int fs_freeze_user = -1;
        bool fs_combo_active = false;
        int fs_combo_tick = 0;
        int fs_combo_atk = 0;
        int fs_combo_vic = 1;
        int fs_combo_clone = 3;
        int fs_combo_home_x = 120;
        int pending_final_smash_victim = -1;
        int pending_final_smash_id = FS_NONE;
        int debug_ai_mode = DBG_AI_NORMAL;
        int debug_ai_target = 1;
        int debug_ai_skill = -1;
        bool net_client = false;
        int debug_force_slot = SLOT_SPECIAL;
        int debug_cd = 0;
        int next_projectile_id = 0;
        int next_block_id = 0;

        // FNV-1a over everything that decides the fight. Both consoles must get the same value after the same
        // number of sim frames; if they don't, the simulation is not deterministic somewhere (lockstep netplay check).
        unsigned state_hash() const
        {
            unsigned h = 2166136261u;
            auto mix = [&h](int v) { h = (h ^ unsigned(v)) * 16777619u; };
            mix(int(rng)); mix(frame); mix(winner);
            for(int i = 0; i < MAX_FIGHTERS; ++i)
            {
                const Fighter& p = f[i];
                mix(p.active ? 1 : 0);
                if(!p.active) { continue; }
                mix(p.x16); mix(p.y16); mix(p.vx); mix(p.vy); mix(p.hp); mix(p.shield); mix(p.smash);
                mix(p.state); mix(p.timer); mix(p.move_frame); mix(p.facing);
                mix(p.har_charge); mix(p.har_power_timer); mix(p.bomb_timer);
            }
            return h;
        }

        static bool on_crafting_table(const Fighter& p)
        {
            return p.has_table &&
                   abs_i(p.x16 - p.table_x16) <= 18 * FP &&
                   abs_i(p.y16 - p.table_y16) <= 22 * FP;
        }

        static int tier_cost(int tier)
        {
            switch(tier)
            {
                case 1: return 3;   // Wood
                case 2: return 10;  // Stone
                case 3: return 30;  // Iron
                case 4: return 50;  // Diamond
                default: return 0;
            }
        }

        static int tier_durability(int tier)
        {
            switch(tier)
            {
                case 1: return 1;   // Wood
                case 2: return 3;   // Stone
                case 3: return 5;   // Iron
                case 4: return 10;  // Diamond
                default: return 0;
            }
        }

        static int next_craft_tier(const Fighter& p)
        {
            for(int t = MAX_WEAPON_TIER; t > p.weapon_tier; --t)
            {
                if(p.materials >= tier_cost(t)) { return t; }
            }
            return 0;
        }

        void apply_final_smash_hit(int attacker_idx, int victim_idx, int damage, int kb_x, int kb_y, int stun,
                                   int heal = 0, int def_bonus = 0, int def_frames = 0)
        {
            ev = Events();
            fs_freeze_timer = 0;
            fs_freeze_user = -1;
            fs_combo_active = false;
            fs_combo_tick = 0;
            for(int i = 0; i < MAX_FIGHTERS; ++i)
            {
                if(!f[i].active) { continue; }
                release_grab(i);
                cancel_move(f[i]);
                f[i].on_pig = false;
                f[i].pig_hp = 0;
                f[i].pig_hits = 0;
                f[i].pig_hit_cd = 0;
                if(f[i].state != ST_KO) { f[i].state = f[i].on_ground ? ST_IDLE : ST_AIR; }
            }
            for(Projectile& p : proj) { p.active = false; }

            Fighter& attacker = f[attacker_idx];
            Fighter& victim = f[victim_idx];
            if(!victim.active || victim.state == ST_KO) { return; }

            if(heal > 0 || def_bonus > 0)
            {
                victim.hp += heal;
                if(victim.hp > victim.stats->max_hp) { victim.hp = victim.stats->max_hp; }
                if(def_bonus > 0)
                {
                    victim.def_buff_amount = def_bonus;
                    victim.def_buff_timer = def_frames;
                }
                spawn_effect(FX_SPARK, victim.x16, victim.y16 - (victim.stats->body_h * FP) / 2, 14);
                ev.absorb = true;
                return;
            }

            int dir = sign_i(victim.x16 - attacker.x16);
            if(dir == 0) { dir = attacker.facing; }
            attacker.facing = dir;

            if(victim.state == ST_SHIELD) { victim.state = ST_IDLE; }

            hit(victim, damage, kb_x, kb_y, stun, dir, victim.x16, victim.y16 - (victim.stats->body_h * FP) / 2, nullptr);
            check_winner();
        }

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
            fs_slow_tick = 0;
            fs_freeze_timer = 0;
            fs_freeze_user = -1;
            fs_combo_active = false;
            fs_combo_tick = 0;
            pending_final_smash_victim = -1;
            pending_final_smash_id = FS_NONE;
            debug_ai_mode = DBG_AI_NORMAL;
            debug_ai_target = 1;
            debug_ai_skill = -1;
            debug_force_slot = SLOT_SPECIAL;
            net_client = false;
            debug_cd = 0;
            next_projectile_id = 0;
            next_block_id = 0;
            for(Projectile& p : proj) { p = Projectile(); }
            for(PlacedBlock& b : blocks) { b = PlacedBlock(); }
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

            for(int i = 0; i < MAX_FIGHTERS; ++i) { f[i] = Fighter(); }
            for(int i = 0; i < count; ++i)
            {
                f[i].active = true;
                f[i].stats = stats[i];
                f[i].x16 = spawn[i] * FP;
                f[i].y16 = geometry.ground_y * FP;
                f[i].on_ground = true;
                f[i].hp = stats[i]->max_hp;
                f[i].jumps_left = stats[i]->max_jumps;
                f[i].facing = team_of(i) == 0 ? 1 : -1;
                f[i].state = ST_IDLE;
                if(stats[i]->spawns_crafting_table)
                {
                    f[i].has_table = true;
                    f[i].table_x16 = spawn[i] * FP;
                    f[i].table_y16 = geometry.ground_y * FP;
                }
            }
        }

        int active_fs_user() const
        {
            for(int i = 0; i < MAX_FIGHTERS; ++i)
            {
                if(f[i].active && f[i].state == ST_ATTACK && f[i].move && f[i].move->final_smash != FS_NONE)
                {
                    return i;
                }
            }
            return -1;
        }

        void step(const Input* in)
        {
            ev = Events();

            if(fs_freeze_timer > 0)
            {
                --fs_freeze_timer;
                if(fs_freeze_timer == 0)
                {
                    fs_freeze_user = -1;
                    ev.swing = true;
                }
                return;
            }

            if(fs_combo_active)
            {
                step_harue_fs();
                return;
            }

            for(int i = 0; i < MAX_FIGHTERS; ++i)
            {
                Fighter& p = f[i];
                if(!p.active || p.is_clone) { continue; }
                const FighterStats& s = *p.stats;
                const bool free_state = p.state == ST_IDLE || p.state == ST_WALK || p.state == ST_AIR || p.state == ST_SHIELD;
                if(winner < 0 && p.state != ST_KO && p.state != ST_GRABBED && free_state &&
                   in[i].final_smash && s.final_smash_move && p.smash >= SMASH_MAX)
                {
                    p.smash = 0;
                    p.on_pig = false;
                    p.pig_hp = 0;
                    p.pig_hits = 0;
                    p.pig_hit_cd = 0;
                    if(in[i].move != 0) { p.facing = in[i].move; }
                    begin_move(p, s.final_smash_move, true);
                    fs_freeze_timer = FS_FREEZE_FRAMES;
                    fs_freeze_user = i;
                    ev.final_smash_start = true;
                    return;
                }
            }

            const int fs_user = active_fs_user();
            bool world_tick = true;
            if(fs_user >= 0)
            {
                ++fs_slow_tick;
                world_tick = (fs_slow_tick % 4) == 0;
            }
            else { fs_slow_tick = 0; }

            for(int i = 0; i < MAX_FIGHTERS; ++i)
            {
                if(!f[i].active) { continue; }
                if(world_tick || i == fs_user) { update_fighter(i, in[i]); }
                if(pending_final_smash >= 0) { return; }
            }
            resolve_melee();
            if(pending_final_smash >= 0) { return; }
            for(int i = 0; i < MAX_FIGHTERS; ++i)
            {
                if(!f[i].active) { continue; }
                if(world_tick || i == fs_user) { advance_move(i); }
            }
            if(world_tick)
            {
                update_grabs();
                update_blocks();
                update_projectiles();
                if(pending_final_smash >= 0) { return; }
                for(Effect& e : fx) { if(e.active && ++e.age >= e.life) { e.active = false; } }
            }
            ++frame;

            if(winner < 0 && frame % SMASH_PASSIVE_FRAMES == 0)
            {
                for(int i = 0; i < MAX_FIGHTERS; ++i)
                {
                    if(f[i].active && !f[i].is_clone && f[i].stats->final_smash_move != nullptr &&
                       f[i].state != ST_KO && f[i].smash < SMASH_MAX)
                    {
                        ++f[i].smash;
                    }
                }
            }

            check_winner();
        }

        // Teams: the player (0) and the partner (2) against the opponent (1).
        bool team_alive(int t) const
        {
            for(int i = 0; i < MAX_FIGHTERS; ++i)
            {
                if(f[i].active && !f[i].is_clone && f[i].hp > 0 && team_of(i) == t) { return true; }
            }
            return false;
        }

        int team_hp(int t) const
        {
            int sum = 0;
            for(int i = 0; i < MAX_FIGHTERS; ++i)
            {
                if(f[i].active && !f[i].is_clone && f[i].hp > 0 && team_of(i) == t) { sum += f[i].hp; }
            }
            return sum;
        }

        void check_winner()
        {
            if(winner >= 0) { return; }
            const bool t0 = team_alive(0);
            const bool t1 = team_alive(1);
            if(!t0)      { winner = 1; }
            else if(!t1) { winner = 0; }
            else if(frame >= TIME_LIMIT) { winner = (team_hp(0) > team_hp(1)) ? 0 : 1; }
        }

        int non_clone_foe_of(int i) const
        {
            const int my_team = team_of(i);
            int best = my_team == 0 ? 1 : 0;
            int best_dist = 0x7FFFFFFF;
            for(int j = 0; j < MAX_FIGHTERS; ++j)
            {
                if(!f[j].active || f[j].is_clone || f[j].state == ST_KO || team_of(j) == my_team) { continue; }
                const int d = abs_i(f[j].x16 - f[i].x16);
                if(d < best_dist) { best_dist = d; best = j; }
            }
            return best;
        }

        static int team_of(int i) { return (i == 1 || i == 4) ? 1 : 0; }

        int foe_of(int i) const
        {
            const int my_team = team_of(i);
            int best_real = -1;
            int best_real_dist = 0x7FFFFFFF;
            int best_any = my_team == 0 ? 1 : 0;
            int best_any_dist = 0x7FFFFFFF;
            for(int j = 0; j < MAX_FIGHTERS; ++j)
            {
                if(!f[j].active || f[j].state == ST_KO || team_of(j) == my_team) { continue; }
                const int d = abs_i(f[j].x16 - f[i].x16);
                if(!f[j].is_clone && d < best_real_dist) { best_real_dist = d; best_real = j; }
                if(d < best_any_dist) { best_any_dist = d; best_any = j; }
            }
            if(best_real >= 0 && (best_any == best_real || best_any_dist > 36 * FP)) { return best_real; }
            return (best_real >= 0 && best_any_dist > best_real_dist) ? best_real : best_any;
        }

        int ally_of(int i) const
        {
            const int my_team = team_of(i);
            int best = -1;
            int best_dist = 0x7FFFFFFF;
            for(int j = 0; j < MAX_FIGHTERS; ++j)
            {
                if(j == i || !f[j].active || f[j].state == ST_KO || team_of(j) != my_team) { continue; }
                const int d = abs_i(f[j].x16 - f[i].x16);
                if(d < best_dist) { best_dist = d; best = j; }
            }
            return best;
        }

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

        Input think(int self)
        {
            Input in;
            const Fighter& me = f[self];
            const Fighter& op = f[foe_of(self)];
            Ai& brain = ai[self];
            if(winner >= 0 || me.state == ST_KO) { return in; }

            if(self == debug_ai_target && debug_ai_mode == DBG_AI_IDLE)
            {
                return in;
            }
            if(self == debug_ai_target && debug_ai_mode == DBG_AI_FORCE)
            {
                const bool can_act_dbg = me.state == ST_IDLE || me.state == ST_WALK || me.state == ST_AIR;
                if(!can_act_dbg) { return in; }
                if(debug_cd > 0) { --debug_cd; return in; }
                if(me.on_pig) { in.jump = true; debug_cd = 10; return in; }

                const int toward_dbg = op.x16 >= me.x16 ? 1 : -1;
                if(debug_force_slot == SLOT_COUNT)
                {
                    if(!me.stats->final_smash_move || op.state == ST_KO) { return in; }
                    if(me.smash < SMASH_MAX) { f[self].smash = SMASH_MAX; }
                    const int adx_dbg = abs_i(op.x16 - me.x16) / FP;
                    const int max_r = (me.stats->final_smash_move->shot_frame >= 0) ? 160 : 44;
                    if(adx_dbg > max_r || me.facing != toward_dbg) { in.move = toward_dbg; return in; }
                    in.final_smash = true;
                    debug_cd = 25;
                    return in;
                }

                if(debug_force_slot == SLOT_COUNT + 1)
                {
                    if(!me.stats->spawns_crafting_table) { return in; }
                    if(me.materials < 1) { f[self].materials = 1; }
                    if(me.on_ground) { in.jump = true; return in; }
                    in.special = true;
                    debug_cd = 25;
                    return in;
                }
                if(debug_force_slot == SLOT_COUNT + 2)
                {
                    if(!me.stats->spawns_crafting_table) { return in; }
                    if(me.weapon_tier >= MAX_WEAPON_TIER) { f[self].weapon_tier = 0; f[self].weapon_dur = 0; }
                    const int cost = tier_cost(me.weapon_tier + 1);
                    if(me.materials < cost) { f[self].materials = cost; }
                    if(!on_crafting_table(me))
                    {
                        in.move = me.table_x16 >= me.x16 ? 1 : -1;
                        return in;
                    }
                    in.special = true;
                    debug_cd = 28;
                    return in;
                }

                const int slot = debug_force_slot;
                if(!has_move(*me.stats, slot)) { return in; }

                const bool is_aerial = slot >= SLOT_NAIR && slot <= SLOT_DAIR;
                if(is_aerial && me.on_ground) { in.jump = true; return in; }

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
            const int skill = (self == debug_ai_target && debug_ai_skill >= 0) ? debug_ai_skill : s.ai_skill;
            const int dx = (op.x16 - me.x16) / FP;
            const int dy = (op.y16 - me.y16) / FP;
            const int adx = abs_i(dx);
            const int ady = abs_i(dy);
            const int toward = dx >= 0 ? 1 : -1;
            const bool can_act = me.state == ST_IDLE || me.state == ST_WALK || me.state == ST_AIR;

            if(brain.drop_hold > 0) { --brain.drop_hold; in.down = true; return in; }

            if(me.state == ST_ATTACK && me.move && me.move->charges_clone)
            {
                if(adx > 42 || ady > 36)
                {
                    in.special_held = true;
                    in.up = true;
                }
                return in;
            }

            // Harue Focus Charge: hold B until charge_hold finishes or enemy rushes inside punish range!
            if(me.state == ST_ATTACK && me.move && me.move->charges_power)
            {
                if(brain.charge_hold > 0)
                {
                    --brain.charge_hold;
                    if(adx < 48 && ady < 34 && random_below(100) < skill)
                    {
                        brain.charge_hold = 0;      // smart cancel: fires Sonic Slash if early, or drops guard
                    }
                    else
                    {
                        in.special_held = true;
                    }
                }
                return in;
            }

            // Deterministic RNG-free AI for Water/Shadow Clones (never touches Fight::rng or desyncs Netplay)
            if(me.is_clone)
            {
                const FighterStats& cs = *me.stats;
                const int cdx = (op.x16 - me.x16) / FP;
                const int cdy = (op.y16 - me.y16) / FP;
                const int cadx = abs_i(cdx);
                const int cady = abs_i(cdy);
                const int ctow = cdx >= 0 ? 1 : -1;
                const bool cact = me.state == ST_IDLE || me.state == ST_WALK || me.state == ST_AIR;
                const int phase = (frame + self * 11) % 60;

                if(me.on_pig)
                {
                    if(me.pig_hp <= 12 || (cdy < -36 && cadx < 60)) { in.jump = true; return in; }
                    in.move = ctow;
                    return in;
                }

                if(cadx > 26) { in.move = ctow; }
                else if(cadx < 12) { in.move = (phase < 20) ? -ctow : 0; }

                if(cact)
                {
                    if(cadx < 34 && cady < 28)
                    {
                        if(me.facing != ctow) { in.move = ctow; }
                        else if(phase % 15 == 0) { in.move = 0; in.attack = true; }
                        else if(phase % 15 == 7) { in.move = ctow; in.attack = true; }
                    }
                    else if(cdy < -24 && cadx < 48 && phase % 18 == 0)
                    {
                        in.up = true;
                        if(cadx < 28) { in.attack = true; }
                        else if(has_move(cs, SLOT_SPECIAL_UP) && !cs.moves->slot[SLOT_SPECIAL_UP]->charges_clone) { in.special = true; }
                    }
                    else if(!me.on_ground && cdy > 20 && cadx < 24 && phase % 12 == 0)
                    {
                        in.down = true; in.attack = true;
                    }
                    else if(cady < 26 && cadx > 48 && phase % 29 == 0)
                    {
                        const Move* side_b = cs.moves ? cs.moves->slot[SLOT_SPECIAL_SIDE] : nullptr;
                        const Move* gun = cs.moves ? cs.moves->slot[SLOT_SPECIAL] : nullptr;
                        if(side_b && cadx > 60 && cadx < 110) { in.move = ctow; in.special = true; }
                        else if(gun && gun->shot_frame >= 0)
                        {
                            if(me.facing == ctow) { in.move = 0; in.special = true; }
                            else                  { in.move = ctow; }
                        }
                    }
                }

                if(!in.attack && !in.special && me.on_ground)
                {
                    if(in.move != 0)
                    {
                        const int lx = me.x16 / FP + in.move * (cs.body_w / 2 + 8);
                        for(const PlacedBlock& bl : blocks)
                        {
                            if(bl.active && lx >= bl.x && lx <= bl.x + bl.w &&
                               me.y16 / FP > bl.y && me.y16 / FP - cs.body_h < bl.y + BLOCK_H)
                            {
                                in.jump = true;
                                break;
                            }
                        }
                    }
                    if(!in.jump && cdy < -30 && phase % 20 == 0) { in.jump = true; }
                    else if(!in.jump && me.support >= 0 && me.support < stage.platform_count && cdy > 32 && phase % 20 == 0)
                    {
                        in.down = true;
                    }
                }
                return in;
            }

            // ---- Adaptive Opponent Observation (scaled by ai_skill) ----
            if(op.state != ST_KO)
            {
                if(op.state == ST_SHIELD && brain.last_opp_state != ST_SHIELD && random_below(100) < skill)
                {
                    if(brain.opp_shield_count < 20) { brain.opp_shield_count += 3; }
                }
                if(op.state == ST_ATTACK && op.move && op.move_frame == 1 && random_below(100) < skill)
                {
                    if(op.move->shot_frame >= 0 || (op.move->shot_list_count > 0))
                    {
                        if(brain.opp_proj_count < 20) { brain.opp_proj_count += 3; }
                    }
                    else if(adx < 52 && brain.opp_rush_count < 20)
                    {
                        brain.opp_rush_count += 2;
                    }
                }
                if(!op.on_ground && (frame % 15 == 0) && random_below(100) < skill && brain.opp_air_count < 20)
                {
                    ++brain.opp_air_count;
                }
                brain.last_opp_state = op.state;
            }
            if(++brain.adapt_timer >= 60)
            {
                brain.adapt_timer = 0;
                if(brain.opp_proj_count > 0)   { --brain.opp_proj_count; }
                if(brain.opp_rush_count > 0)   { --brain.opp_rush_count; }
                if(brain.opp_shield_count > 0) { --brain.opp_shield_count; }
                if(brain.opp_air_count > 0)    { --brain.opp_air_count; }
            }

            // 0. TNT awareness
            for(const Projectile& t : proj)
            {
                if(!t.active || t.shot.kind != PROJ_TNT) { continue; }
                const int tdx = (t.x16 - me.x16) / FP;
                const int tdy = abs_i((t.y16 + 8 * FP - me.y16) / FP);
                if(abs_i(tdx) < 30 && tdy < 26)
                {
                    in.move = tdx >= 0 ? -1 : 1;
                    if(me.on_ground && (t.age >= TNT_ARM_FRAMES - 4) && random_below(100) < 35) { in.jump = true; }
                    else if(me.on_pig) { in.jump = true; }
                    return in;
                }
            }

            // 0b. Pig mount
            if(me.on_pig)
            {
                if(me.pig_hp <= 12 || (dy < -36 && adx < 60) || (me.smash >= SMASH_MAX && s.final_smash_move))
                {
                    in.jump = true;
                    return in;
                }
                in.move = toward;
                return in;
            }

            // 0c. Final Smash (only target the real opponent, never a clone!)
            const int real_foe_idx = non_clone_foe_of(self);
            const Fighter& real_foe = f[real_foe_idx];
            if(can_act && s.final_smash_move && me.smash >= SMASH_MAX && real_foe.active && !real_foe.is_clone && real_foe.state != ST_KO)
            {
                const int fs_dx = (real_foe.x16 - me.x16) / FP;
                const int fs_adx = abs_i(fs_dx);
                const int fs_ady = abs_i((real_foe.y16 - me.y16) / FP);
                const int fs_tow = fs_dx >= 0 ? 1 : -1;
                const bool ranged_fs = s.final_smash_move->shot_frame >= 0;
                const int max_dx = ranged_fs ? 175 : 52;
                const int max_dy = ranged_fs ? 26 : 32;
                if(fs_adx < max_dx && fs_ady < max_dy)
                {
                    if(me.facing != fs_tow) { in.move = fs_tow; return in; }
                    if(random_below(100) < 35 + skill / 2)
                    {
                        in.final_smash = true;
                        return in;
                    }
                }
            }

            // 1. Incoming projectile reaction!
            const Projectile* threat = nullptr;
            int threat_dist = 9999;
            const int my_mid_y = me.y16 - (s.body_h * FP) / 2;
            for(const Projectile& p : proj)
            {
                if(!p.active || (team_of(p.owner) == team_of(self) && !p.shot.friendly_fire)) { continue; }
                const bool is_tnt = (p.shot.kind == PROJ_TNT && p.vx == 0 && team_of(p.owner) != team_of(self));
                if(!is_tnt && p.vx == 0 && p.vy == 0) { continue; }
                const int pdx = (p.x16 - me.x16) / FP;
                const bool coming = is_tnt ? (abs_i(pdx) < 44)
                                           : ((pdx > 0 && p.vx < 0) || (pdx < 0 && p.vx > 0) || (p.vx == 0 && abs_i(pdx) < 20));
                const int pdy = abs_i((p.y16 - my_mid_y) / FP);
                if(coming && abs_i(pdx) < 108 && pdy < s.body_h / 2 + p.shot.h / 2 + 6 && abs_i(pdx) < threat_dist)
                {
                    threat = &p;
                    threat_dist = abs_i(pdx);
                }
            }

            const Move* down_b = s.moves ? s.moves->slot[SLOT_SPECIAL_DOWN] : nullptr;
            const bool can_reflect = down_b && down_b->reflect_end > 0 && me.har_power_timer == 0;
            const bool can_absorb  = down_b && down_b->absorb_end > 0 && !me.has_stored;

            if(threat && threat_dist < 80 && threat->id != brain.absorb_handled)
            {
                brain.absorb_handled = threat->id;
                const int react_bonus = (brain.opp_proj_count >= 5) ? 25 : 0;
                const int reflect_chance = skill * 3 / 4 + react_bonus;
                if(can_act && threat->shot.absorbable && (can_reflect || can_absorb) &&
                   random_below(100) < (can_reflect ? reflect_chance : (skill / 2 + react_bonus)))
                {
                    brain.handled_projectile = threat->id;
                    in.special = true;
                    in.down = true;
                    return in;
                }
            }

            if(threat && threat->id != brain.handled_projectile && (threat->vx != 0 || threat->vy != 0))
            {
                brain.handled_projectile = threat->id;
                if(me.on_ground && random_below(100) < skill)          { brain.shield_hold = 16 + random_below(10); }
                else if(me.jumps_left > 0 && random_below(100) < 40 + skill / 4) { in.jump = true; }
            }
            if(brain.shield_hold > 0)
            {
                --brain.shield_hold;
                in.shield = true;
                return in;
            }

            // 1b. Melee defense!
            if(me.on_ground && can_act && op.state == ST_ATTACK && op.move && op.move->hit_count > 0 &&
               adx < 48 && ady < 34 && frame - brain.last_react > 22)
            {
                brain.last_react = frame;
                const bool has_melee_counter = down_b && down_b->on_counter_melee != nullptr;
                const int rush_bonus = (brain.opp_rush_count >= 6) ? 20 : 0;
                if(has_melee_counter && random_below(100) < skill / 3 + rush_bonus)
                {
                    if(me.facing != toward) { in.move = toward; return in; }
                    in.special = true;
                    in.down = true;
                    return in;
                }
                if(random_below(100) < skill / 2)
                {
                    brain.shield_hold = 14 + random_below(10);
                }
            }
            if(brain.shield_hold > 0)
            {
                --brain.shield_hold;
                in.shield = true;
                return in;
            }

            // 2. Stored shot throw (Waffles Fox Thief)
            if(me.has_stored && can_act && down_b && down_b->alt && ady < 26 && adx < 170 && random_below(100) < 8 + skill / 10)
            {
                if(me.facing != toward) { in.move = toward; return in; }
                in.special = true;
                in.down = true;
                return in;
            }

            // 2a. Harue strategic abilities: Focus Charge (Hold B) & Shadow Twin Clone (Powered Down+B)
            const Move* neut_b = s.moves ? s.moves->slot[SLOT_SPECIAL] : nullptr;
            if(neut_b && neut_b->charges_power && can_act)
            {
                if(me.har_power_timer > 0)
                {
                    const int cl_slot = (team_of(self) == 0) ? 3 : 4;
                    if(!f[cl_slot].active && down_b && down_b->alt && adx > 48 &&
                       (me.har_power_timer < 240 || random_below(1000) < 12 + skill / 6))
                    {
                        in.special = true;
                        in.down = true;
                        return in;
                    }
                }
                else if(me.on_ground && adx > 82 && skill >= 30 && random_below(1000) < 8 + skill / 5)
                {
                    brain.charge_hold = 35 + random_below(55);
                    in.special = true;
                    in.special_held = true;
                    return in;
                }
            }

            // 2b. Umbreon mining & crafting priority
            if(s.spawns_crafting_table && can_act && me.on_ground && me.weapon_tier < MAX_WEAPON_TIER && adx > 46)
            {
                const int can_tier = next_craft_tier(me);
                if(can_tier > 0 && (adx > 65 || me.weapon_tier == 0 || can_tier >= me.weapon_tier + 2))
                {
                    if(on_crafting_table(me))
                    {
                        in.special = true;
                        return in;
                    }
                    else
                    {
                        const int to_table = me.table_x16 >= me.x16 ? 1 : -1;
                        if(sign_i(dx) != to_table || adx > 65)
                        {
                            in.move = to_table;
                            return in;
                        }
                    }
                }
                else if(me.materials < MAX_MATERIALS && adx > 58 && random_below(100) < 15 + skill / 8)
                {
                    in.special = true;
                    return in;
                }
            }

            // 2c. Umbreon midair block placement
            if(s.spawns_crafting_table && can_act && !me.on_ground && me.materials >= 1 &&
               me.vy >= 0 && me.y16 < (stage.ground_y - 16) * FP &&
               random_below(100) < 10 + skill / 10)
            {
                in.special = true;
                return in;
            }

            // 3. Movement (adaptive spacing & stun-punish rushdown)
            if(--brain.mode_timer <= 0)
            {
                brain.mode_timer = 75 + random_below(130);
                const int rush_bias = (brain.opp_proj_count >= 6 ? 25 : 0) - (brain.opp_rush_count >= 8 ? 15 : 0);
                brain.rush = random_below(100) < (40 + skill / 3 + rush_bias);
            }
            const bool op_stunned = (op.state == ST_HURT && op.timer >= 12);
            const bool seeking_fs = s.final_smash_move != nullptr && me.smash >= SMASH_MAX &&
                                    s.final_smash_move->shot_frame < 0 && real_foe.active && real_foe.state != ST_KO;
            const bool has_zoning = (s.moves && s.moves->slot[SLOT_SPECIAL_UP] && s.moves->slot[SLOT_SPECIAL_UP]->shot_list_count > 0) ||
                                    (neut_b && (neut_b->shot_frame >= 0 || neut_b->shot_list_count > 0 || neut_b->charges_power));
            const int move_dx = seeking_fs ? ((real_foe.x16 - me.x16) / FP) : dx;
            const int move_adx = abs_i(move_dx);
            const int move_tow = move_dx >= 0 ? 1 : -1;
            const int wanted_range = (seeking_fs || op_stunned) ? 18 : (brain.rush ? 24 : (has_zoning ? 88 : 42));
            if(--brain.strafe_timer <= 0)
            {
                brain.strafe_timer = 22 + random_below(45);
                brain.strafe = random_below(3) - 1;
            }
            if(move_adx > wanted_range + 22)       { in.move = move_tow; }
            else if(move_adx < wanted_range - 36)  { in.move = -move_tow; }
            else                              { in.move = brain.strafe; }

            // 4. Attacks (punishes stuns, beats shields, uses full kit!)
            if(can_act)
            {
                const bool grounded = me.on_ground;
                const bool facing_op = me.facing == toward;
                const int urge = 2 + skill / 18 + (brain.rush ? 3 : 0) + (op_stunned ? 12 : 0);
                const Move* side_b = s.moves ? s.moves->slot[SLOT_SPECIAL_SIDE] : nullptr;
                const Move* up_b   = s.moves ? s.moves->slot[SLOT_SPECIAL_UP] : nullptr;
                const Move* fsmash = s.moves ? s.moves->slot[SLOT_FSMASH] : nullptr;
                const Move* dsmash = s.moves ? s.moves->slot[SLOT_DSMASH] : nullptr;

                // Stun punish! (e.g. Filip after landing Insult, or Harue after Sonic Slash / shield break)
                if(op_stunned && adx < 56 && ady < 28 && random_below(100) < 20 + skill / 2)
                {
                    if(grounded && fsmash && adx < 46)
                    {
                        in.move = toward; in.attack = true; return in;
                    }
                    if(side_b && side_b->on_grab)
                    {
                        in.move = toward; in.special = true; return in;
                    }
                }

                // Shield pressure! If opponent shields a lot, grab (Filip Pounce) or space them out
                if(op.state == ST_SHIELD && adx < 54 && ady < 28 && random_below(100) < 8 + skill / 5 + brain.opp_shield_count)
                {
                    if(side_b && side_b->on_grab)
                    {
                        in.move = toward; in.special = true; return in;
                    }
                    if(grounded && dsmash && adx < 28)
                    {
                        in.down = true; in.attack = true; return in;
                    }
                }

                if(adx < 42 && ady < 30)
                {
                    if(random_below(100) < urge + 5)
                    {
                        const int r = random_below(100);
                        if(grounded)
                        {
                            if(s.spawns_crafting_table && !facing_op && dsmash && r < 55)
                            {
                                in.down = true; in.attack = true;
                            }
                            else if(adx > 26 && fsmash && fsmash->hit_count > 0 && fsmash->hits[0].w >= 36 && r < 65)
                            {
                                // Long-reach Side+A spacing (e.g. Harue Great Cleave)
                                in.move = toward; in.attack = true;
                            }
                            else if(r < 35)
                            {
                                if(facing_op) { in.move = 0; in.attack = true; } else { in.move = toward; }
                            }
                            else if(r < 68) { in.move = toward; in.attack = true; }
                            else if(r < 88) { in.down = true; in.attack = true; }
                            else if(s.spawns_crafting_table && down_b)
                            {
                                in.down = true; in.special = true;
                            }
                        }
                        else
                        {
                            if(r < 50) { in.move = 0; in.attack = true; }
                            else       { in.move = toward; in.attack = true; }
                        }
                    }
                }
                else if(up_b && up_b->charges_clone && !me.is_clone && me.hp > CLONE_HP_COST + 10 &&
                        !f[team_of(self) == 0 ? 3 : 4].active && adx > 95 && random_below(1000) < 12)
                {
                    in.up = true; in.special = true;
                }
                else if(dy < -22 && adx < 55)
                {
                    const int r = random_below(100);
                    const int aa_urge = urge + (brain.opp_air_count >= 5 ? 4 : 0);
                    if(adx < 30 && r < aa_urge + 4)
                    {
                        if(!facing_op) { in.move = toward; }
                        else           { in.up = true; in.attack = true; }
                    }
                    else if(up_b && !up_b->charges_clone && up_b->shot_list_count == 0 && up_b->shot_frame < 0 &&
                            adx < 28 && dy > -64 && r < aa_urge + 3)
                    {
                        // Only commit to aerial Up+B (Aqua Jet Up / Sky Splitter) when cleanly lined up!
                        if(!facing_op) { in.move = toward; }
                        else           { in.up = true; in.special = true; }
                    }
                }
                else if(!grounded && dy > 18 && adx < 26)
                {
                    if(random_below(100) < urge + 5) { in.down = true; in.attack = true; }
                }
                else if(ady < 32)
                {
                    const Move* jab_m = s.moves ? s.moves->slot[SLOT_JAB] : nullptr;
                    const int gun_range = (neut_b && neut_b->shot_frame >= 0) ? neut_b->shot.speed * neut_b->shot.life / FP : 0;

                    // Filip Throw Junk (B+Up) at mid-range (facing opponent!)
                    if(up_b && up_b->shot_list_count > 0 && grounded && adx >= 44 && adx <= 135 && random_below(100) < 4 + skill / 14)
                    {
                        if(!facing_op) { in.move = toward; }
                        else           { in.up = true; in.special = true; }
                    }
                    // Harue Tap B Sonic Slash (when neut_b has charges_power + alt projectile!)
                    else if(neut_b && neut_b->charges_power && neut_b->alt && adx >= 52 && adx <= 175 && random_below(100) < 4 + skill / 12)
                    {
                        if(!facing_op) { in.move = toward; }
                        else           { in.move = 0; in.special = true; in.special_held = false; }
                    }
                    else if(s.spawns_crafting_table && grounded && adx >= 36 && adx <= 72 && op.vx * dx < 0 &&
                            down_b && random_below(1000) < 12 + skill / 8)
                    {
                        in.down = true; in.special = true;
                    }
                    else if(jab_m && jab_m->shot_frame >= 0 && grounded && adx > 34 && adx < 115 && random_below(100) < 4 + skill / 12)
                    {
                        if(facing_op) { in.move = 0; in.attack = true; }
                        else          { in.move = toward; }
                    }
                    else
                    {
                        const int jet_range = (!side_b || side_b->mounts_pig) ? 0
                                            : (side_b->leap_frame >= 0
                                               ? side_b->leap_vx * 16 / FP + 16
                                               : side_b->drive.vx * (side_b->drive.end - side_b->drive.start) / FP + 12);
                        const int gun_reach = gun_range > 0 ? (gun_range > 85 ? gun_range : 85) : 0;

                        if(side_b && side_b->mounts_pig && adx > 65 && random_below(1000) < 10 + skill / 8)
                        {
                            in.move = toward; in.special = true;
                        }
                        else if(jet_range > 0 && adx > 38 && adx < jet_range &&
                                random_below(1000) < (me.har_power_timer > 0 ? 18 : 7) + skill / 8)
                        {
                            in.move = toward; in.special = true;
                        }
                        else if(gun_reach > 0 && adx > 36 && adx < gun_reach && random_below(100) < 4 + skill / 12)
                        {
                            if(facing_op) { in.move = 0; in.special = true; }
                            else          { in.move = toward; }
                        }
                        else if(neut_b && neut_b->shot.heal_ally > 0)
                        {
                            const int al = ally_of(self);
                            if(al >= 0 && f[al].hp < f[al].stats->max_hp * 85 / 100)
                            {
                                const int al_dx = (f[al].x16 - me.x16) / FP;
                                const int al_dy = abs_i((f[al].y16 - me.y16) / FP);
                                if(abs_i(al_dx) < 180 && al_dy < 26 && random_below(100) < 8)
                                {
                                    const int toward_al = al_dx >= 0 ? 1 : -1;
                                    if(me.facing == toward_al) { in.move = 0; in.special = true; }
                                    else                       { in.move = toward_al; }
                                }
                            }
                        }
                    }
                }
            }

            // 5. Vertical navigation + jumping over placed block walls
            if(!in.attack && !in.special)
            {
                if(me.on_ground && in.move != 0)
                {
                    const int look_x = me.x16 / FP + in.move * (s.body_w / 2 + 8);
                    const int my_top = me.y16 / FP - s.body_h;
                    const int my_bot = me.y16 / FP;
                    for(const PlacedBlock& bl : blocks)
                    {
                        if(!bl.active) { continue; }
                        if(look_x >= bl.x && look_x <= bl.x + bl.w && my_bot > bl.y && my_top < bl.y + BLOCK_H)
                        {
                            in.jump = true;
                            break;
                        }
                    }
                }
                if(!in.jump)
                {
                    if(seeking_fs && me.on_ground && dy < -24 && random_below(100) < 12) { in.jump = true; }
                    else if(me.on_ground && dy < -36 && random_below(100) < 5)           { in.jump = true; }
                    else if(me.on_ground && me.support >= 0 && me.support < stage.platform_count && dy > 36 && random_below(100) < 4)
                    {
                        brain.drop_hold = DROP_HOLD + 2;
                        in.down = true;
                    }
                    else if(me.on_ground && random_below(1000) < 4)                      { in.jump = true; }
                }
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

        static int scale_weapon_dmg(const Fighter& atk, int dmg)
        {
            switch(atk.weapon_tier)
            {
                case 1: return dmg * 120 / 100;   // Wood: 1.2x
                case 2: return dmg * 145 / 100;   // Stone: 1.45x
                case 3: return dmg * 175 / 100;   // Iron: 1.75x
                case 4: return dmg * 220 / 100;   // Diamond: 2.2x
                default: return dmg;
            }
        }

        static int scale_weapon_kb(const Fighter& atk, int kb)
        {
            switch(atk.weapon_tier)
            {
                case 1: return kb * 108 / 100;
                case 2: return kb * 116 / 100;
                case 3: return kb * 125 / 100;
                case 4: return kb * 138 / 100;
                default: return kb;
            }
        }

        void use_weapon_durability(Fighter& atk)
        {
            if(atk.weapon_tier > 0 && atk.weapon_dur > 0)
            {
                if(--atk.weapon_dur <= 0)
                {
                    atk.weapon_tier = 0;
                    atk.weapon_dur = 0;
                    spawn_effect(FX_DUST, atk.x16 + atk.facing * 10 * FP, atk.y16 - 14 * FP, 16);
                }
            }
        }
        void update_fighter(int i, const Input& raw)
        {
            Fighter& p = f[i];
            const FighterStats& s = *p.stats;
            Input in = raw;
            if(winner >= 0 || p.state == ST_KO || p.state == ST_GRABBED) { in = Input(); }
            if(p.drop_timer > 0) { --p.drop_timer; }
            if(p.def_buff_timer > 0 && --p.def_buff_timer == 0) { p.def_buff_amount = 0; }
            if(p.har_power_timer > 0) { --p.har_power_timer; }
            if(p.bomb_timer > 0 && --p.bomb_timer == 0)
            {
                const int own = (p.bomb_owner >= 0 && p.bomb_owner < MAX_FIGHTERS) ? p.bomb_owner : (team_of(i) == 0 ? 1 : 0);
                const Shot boom = Shot().of(PROJ_EXPLOSION).dmg(p.bomb_dmg > 0 ? p.bomb_dmg : 14).fly(0, 12).size(42, 34)
                                        .launch(fp(4.2), fp(-4.2)).stun(24).steal_proof().lingers();
                spawn_shot_at(own, boom, p.x16, p.y16 - (s.body_h * FP) / 2, 0, false);
                ev.boom = true;
                p.bomb_owner = -1;
                p.bomb_dmg = 0;
            }
            if(!net_client && p.is_clone && p.clone_timer > 0 && --p.clone_timer == 0)
            {
                if(p.holding >= 0) { release_grab(p.holding); }
                cancel_move(p);
                p.hp = 0;
                p.active = false;
                p.state = ST_KO;
                spawn_effect(FX_DUST, p.x16, p.y16 - 12 * FP, 16);
                return;
            }

            if(p.on_pig)
            {
                if(in.final_smash && s.final_smash_move && p.smash >= SMASH_MAX)
                {
                    p.on_pig = false;
                    p.pig_hp = 0;
                    p.pig_hits = 0;
                    p.pig_hit_cd = 0;
                }
                else if(in.jump)
                {
                    p.on_pig = false;
                    p.pig_hp = 0;
                    p.pig_hits = 0;
                    p.pig_hit_cd = 0;
                    p.vy = -s.jump_power;
                    p.on_ground = false;
                    p.support = -1;
                    p.state = ST_AIR;
                    p.jumps_left = s.max_jumps > 1 ? s.max_jumps - 1 : 0;
                    p.down_hold = 0;
                    ev.jump = true;
                    integrate(p, false, false);
                    return;
                }
                else
                {
                    if(in.move != 0) { p.facing = in.move; }
                    const int pig_speed = (in.move == p.facing && in.move != 0) ? fp(3.2) : fp(2.3);
                    p.vx = approach(p.vx, p.facing * pig_speed, fp(0.5));
                    p.state = p.on_ground ? ST_WALK : ST_AIR;
                    integrate(p, in.down, false);
                    if(p.x16 <= BODY_MARGIN * FP || p.x16 >= (stage.length - BODY_MARGIN) * FP)
                    {
                        p.facing = -p.facing;
                    }
                    if(p.pig_hit_cd > 0) { --p.pig_hit_cd; }
                    else
                    {
                        const Rect pig_box{ p.x16 / FP, p.y16 / FP - 8, 28, 16 };
                        for(int v = 0; v < MAX_FIGHTERS; ++v)
                        {
                            if(!f[v].active || team_of(v) == team_of(i) || f[v].state == ST_KO) { continue; }
                            if(overlaps(pig_box, hurtbox_of(f[v])))
                            {
                                const int dir = sign_i(f[v].x16 - p.x16) != 0 ? sign_i(f[v].x16 - p.x16) : p.facing;
                                hit(f[v], 6, fp(3.2), fp(-2.4), 18, dir, f[v].x16, f[v].y16 - 10 * FP, &p);
                                p.pig_hit_cd = PIG_HIT_CD;
                                if(++p.pig_hits >= PIG_MAX_HITS)
                                {
                                    p.on_pig = false;
                                    p.pig_hp = 0;
                                    p.pig_hits = 0;
                                    p.pig_hit_cd = 0;
                                    p.vx /= 2;
                                    p.state = p.on_ground ? ST_IDLE : ST_AIR;
                                    spawn_effect(FX_DUST, p.x16 - 8 * FP, p.y16 - 8 * FP, 18);
                                    spawn_effect(FX_DUST, p.x16 + 8 * FP, p.y16 - 10 * FP, 20);
                                }
                                break;
                            }
                        }
                    }
                    return;
                }
            }

            // ---- states that run by themselves ----
            bool floating = false;                                        // true while an aqua jet is flying
            if(p.state == ST_ATTACK && p.move && p.move->charges_power)
            {
                ++p.move_frame;
                if(!in.special_held || winner >= 0)
                {
                    if(p.move_frame <= 10 && p.move->alt && winner < 0)
                    {
                        begin_move(p, p.move->alt, true);
                        ev.swing = true;
                        return;
                    }
                    p.move = nullptr;
                    p.move_frame = 0;
                    p.state = p.on_ground ? ST_IDLE : ST_AIR;
                }
                else
                {
                    if(p.move_frame >= 10)
                    {
                        ++p.har_charge;
                        if(p.har_charge >= HAR_CHARGE_MAX)
                        {
                            p.har_charge = 0;
                            p.har_power_timer = HAR_POWER_FRAMES;
                            p.move = nullptr;
                            p.move_frame = 0;
                            p.state = p.on_ground ? ST_IDLE : ST_AIR;
                            spawn_effect(FX_SPARK, p.x16, p.y16 - (s.body_h * FP) / 2, 14);
                            ev.absorb = true;
                        }
                    }
                    if(p.on_ground) { p.vx = approach(p.vx, 0, s.friction); }
                    integrate(p, false, false);
                    return;
                }
            }

            if(p.state == ST_ATTACK && p.move && p.move->charges_clone)
            {
                if(!in.special_held || winner >= 0)
                {
                    p.move = nullptr;
                    p.move_frame = 0;
                    p.state = p.on_ground ? ST_IDLE : ST_AIR;
                }
                else
                {
                    ++p.clone_charge;
                    p.move_frame = p.clone_charge;
                    if(p.clone_charge >= CLONE_CHARGE_MAX)
                    {
                        p.clone_charge = 0;
                        p.move = nullptr;
                        p.move_frame = 0;
                        p.state = p.on_ground ? ST_IDLE : ST_AIR;
                        if(!net_client && p.hp > CLONE_HP_COST) { spawn_water_clone(i); }
                    }
                    if(p.on_ground) { p.vx = approach(p.vx, 0, s.friction); }
                    integrate(p, false, false);
                    return;
                }
            }

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
                    if(s.final_smash_move->hit_count > 0 || s.final_smash_move->drive.end > s.final_smash_move->drive.start || s.final_smash_move->shot_frame >= 0) { ev.swing = true; }
                    ev.final_smash_start = true;
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
                    else if(in.down && p.on_ground && p.support >= 0 && p.support < stage.platform_count && p.state != ST_SHIELD)
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
                int wspd = s.walk_speed;
                int accel = p.on_ground ? s.ground_accel : s.air_accel;
                if(p.har_power_timer > 0)
                {
                    wspd = wspd * 135 / 100;
                    accel = accel * 135 / 100;
                }
                const int target = in.move * wspd;
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
            if(m->charges_clone && p.hp <= CLONE_HP_COST) { return false; }
            if(slot == SLOT_SPECIAL && s.spawns_crafting_table)
            {
                if(!p.on_ground)
                {
                    if(p.materials < 1) { return false; }
                    if(m->alt && m->alt->alt) { m = m->alt->alt; }
                }
                else if(on_crafting_table(p) && next_craft_tier(p) > 0)
                {
                    if(m->alt) { m = m->alt; }
                }
            }
            else if(m->alt && p.has_stored) { m = m->alt; }
            else if(slot == SLOT_SPECIAL_DOWN && m->alt && p.har_power_timer > 0)
            {
                m = m->alt;
                p.har_power_timer = 0;
            }
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

            const Drive& d = (m.drive2.end > m.drive2.start && t >= m.drive2.start && t < m.drive2.end) ? m.drive2 : m.drive;
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

            if(m.tech_frame >= 0 && t == m.tech_frame)
            {
                if(m.mines)
                {
                    p.materials += MINE_GAIN;
                    if(p.materials > MAX_MATERIALS) { p.materials = MAX_MATERIALS; }
                    ev.mining = true;
                }
                if(m.crafts)
                {
                    const int tier = next_craft_tier(p);
                    if(tier > 0)
                    {
                        p.materials -= tier_cost(tier);
                        p.weapon_tier = tier;
                        p.weapon_dur = tier_durability(tier);
                        ev.absorb = true;
                    }
                }
                if(m.places_block) { spawn_block(i); ev.mining = true; }
                if(m.places_tnt)   { spawn_tnt(i); ev.shot = true; }
                if(m.summons_har_clone && !net_client) { spawn_har_clone(i); }
                if(m.mounts_pig)
                {
                    p.on_pig = true;
                    p.pig_hp = PIG_MAX_HP;
                    p.pig_hits = 0;
                    p.pig_hit_cd = 0;
                    p.move = nullptr;
                    p.move_frame = 0;
                    p.state = p.on_ground ? ST_WALK : ST_AIR;
                    ev.dash = true;
                    return false;
                }
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
            if(p.state != ST_ATTACK || !p.move || p.move->charges_clone) { return; }
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

            if(!floating)
            {
                p.vy += s.gravity;
                const int limit = (holding_down && p.state != ST_KO) ? s.fast_fall : s.max_fall;
                if(p.vy > limit) { p.vy = limit; }
            }

            const int old_x16 = p.x16;
            const int old_y16 = p.y16;
            p.x16 += p.vx;

            const int min_x = BODY_MARGIN * FP;
            const int max_x = (stage.length - BODY_MARGIN) * FP;
            if(p.x16 < min_x) { p.x16 = min_x; p.vx = 0; }
            if(p.x16 > max_x) { p.x16 = max_x; p.vx = 0; }

            // Solid block horizontal wall collision (blocks enemies & fighters from walking through)
            const int hw16 = (s.body_w * FP) / 2;
            const int bh16 = s.body_h * FP;
            for(int k = 0; k < MAX_BLOCKS; ++k)
            {
                const PlacedBlock& bl = blocks[k];
                if(!bl.active) { continue; }
                const int bL = bl.x * FP;
                const int bR = (bl.x + bl.w) * FP;
                const int bT = bl.y * FP;
                const int bB = (bl.y + BLOCK_H) * FP;
                if(old_y16 > bT + FP && old_y16 - bh16 < bB - FP)
                {
                    if(p.vx > 0 && old_x16 + hw16 <= bL + 4 * FP && p.x16 + hw16 > bL)
                    {
                        p.x16 = bL - hw16;
                        p.vx = 0;
                        if(p.on_pig) { p.facing = -1; }
                    }
                    else if(p.vx < 0 && old_x16 - hw16 >= bR - 4 * FP && p.x16 - hw16 < bR)
                    {
                        p.x16 = bR + hw16;
                        p.vx = 0;
                        if(p.on_pig) { p.facing = 1; }
                    }
                }
            }

            p.y16 += p.vy;

            // Solid block ceiling headbonk
            if(p.vy < 0)
            {
                for(int k = 0; k < MAX_BLOCKS; ++k)
                {
                    const PlacedBlock& bl = blocks[k];
                    if(!bl.active) { continue; }
                    const int bL = bl.x * FP;
                    const int bR = (bl.x + bl.w) * FP;
                    const int bB = (bl.y + BLOCK_H) * FP;
                    if(p.x16 + hw16 > bL + FP && p.x16 - hw16 < bR - FP &&
                       old_y16 - bh16 >= bB - 4 * FP && p.y16 - bh16 < bB)
                    {
                        p.y16 = bB + bh16;
                        p.vy = 0;
                    }
                }
            }

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

                // Solid blocks always support feet on top (cannot drop through)
                for(int k = 0; k < MAX_BLOCKS; ++k)
                {
                    const PlacedBlock& bl = blocks[k];
                    if(!bl.active) { continue; }
                    const int bL = bl.x * FP;
                    const int bR = (bl.x + bl.w) * FP;
                    const int top16 = bl.y * FP;
                    if(p.x16 + hw16 > bL + FP && p.x16 - hw16 < bR - FP &&
                       old_y16 <= top16 + 4 * FP && p.y16 >= top16 && (!landed || top16 < best_y16))
                    {
                        landed = true;
                        best_y16 = top16;
                        support = 100 + k;
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

        void spawn_water_clone(int owner)
        {
            const int team = team_of(owner);
            const int slot = (team == 0) ? 3 : 4;
            int src = foe_of(owner);
            if(f[src].is_clone && f[src].clone_of >= 0) { src = f[src].clone_of; }
            if(!f[src].active || !f[src].stats) { return; }

            FighterStats& cs = clone_stats[team];
            cs = *f[src].stats;
            cs.max_hp = cs.max_hp / 10;
            if(cs.max_hp < 1) { cs.max_hp = 1; }
            cs.phys_pct = cs.phys_pct / 2;
            cs.magic_pct = cs.magic_pct / 2;
            cs.final_smash_move = nullptr;
            cs.spawns_crafting_table = false;

            Fighter& op = f[owner];
            if(op.hp <= CLONE_HP_COST) { return; }
            op.hp -= CLONE_HP_COST;
            int cx = op.x16 / FP + op.facing * 24;
            if(cx < BODY_MARGIN) { cx = BODY_MARGIN; }
            if(cx > stage.length - BODY_MARGIN) { cx = stage.length - BODY_MARGIN; }

            f[slot] = Fighter();
            f[slot].active = true;
            f[slot].is_clone = true;
            f[slot].clone_of = src;
            f[slot].stats = &cs;
            f[slot].x16 = cx * FP;
            f[slot].y16 = op.y16;
            f[slot].facing = op.facing;
            f[slot].hp = cs.max_hp;
            f[slot].clone_timer = CLONE_LIFE_FRAMES;
            f[slot].jumps_left = cs.max_jumps;
            f[slot].on_ground = op.on_ground;
            f[slot].state = op.on_ground ? ST_IDLE : ST_AIR;
            ai[slot] = Ai();

            spawn_effect(FX_SPARK, f[slot].x16, f[slot].y16 - 14 * FP, 12);
            ev.absorb = true;
            ev.dash = true;
        }

    public:
        void spawn_water_clone_from_net(int slot, int src, int facing, int hp)
        {
            if(slot < 3 || slot >= MAX_FIGHTERS || src < 0 || src >= 3 || !f[src].active || !f[src].stats) { return; }
            const int team = team_of(slot);
            FighterStats& cs = clone_stats[team];
            cs = *f[src].stats;
            cs.max_hp = cs.max_hp / 10;
            if(cs.max_hp < 1) { cs.max_hp = 1; }
            cs.phys_pct = cs.phys_pct / 2;
            cs.magic_pct = cs.magic_pct / 2;
            cs.final_smash_move = nullptr;
            cs.spawns_crafting_table = false;

            const int owner = (team == 0) ? 0 : 1;
            const Fighter& op = f[owner];
            int cx = op.x16 / FP + facing * 24;
            if(cx < BODY_MARGIN) { cx = BODY_MARGIN; }
            if(cx > stage.length - BODY_MARGIN) { cx = stage.length - BODY_MARGIN; }

            f[slot] = Fighter();
            f[slot].active = true;
            f[slot].is_clone = true;
            f[slot].clone_of = src;
            f[slot].stats = &cs;
            f[slot].x16 = cx * FP;
            f[slot].y16 = op.y16;
            f[slot].facing = facing;
            f[slot].hp = (hp > 0 && hp <= cs.max_hp) ? hp : cs.max_hp;
            f[slot].clone_timer = CLONE_LIFE_FRAMES;
            f[slot].jumps_left = cs.max_jumps;
            f[slot].on_ground = op.on_ground;
            f[slot].state = op.on_ground ? ST_IDLE : ST_AIR;
            ai[slot] = Ai();

            spawn_effect(FX_SPARK, f[slot].x16, f[slot].y16 - 14 * FP, 12);
            ev.absorb = true;
            ev.dash = true;
        }

        void spawn_har_clone(int owner)
        {
            const int team = team_of(owner);
            const int slot = (team == 0) ? 3 : 4;
            const bool use_waffle = (waffle_ref_stats != nullptr) && (random_below(2) == 0);
            const FighterStats* base_s = use_waffle ? waffle_ref_stats : f[owner].stats;
            if(!base_s) { return; }

            FighterStats& cs = clone_stats[team];
            cs = *base_s;
            cs.max_hp = cs.max_hp / 10;
            if(cs.max_hp < 1) { cs.max_hp = 1; }
            cs.phys_pct = cs.phys_pct / 2;
            cs.magic_pct = cs.magic_pct / 2;
            cs.final_smash_move = nullptr;
            cs.spawns_crafting_table = false;

            const Fighter& op = f[owner];
            int cx = op.x16 / FP + op.facing * 24;
            if(cx < BODY_MARGIN) { cx = BODY_MARGIN; }
            if(cx > stage.length - BODY_MARGIN) { cx = stage.length - BODY_MARGIN; }

            f[slot] = Fighter();
            f[slot].active = true;
            f[slot].is_clone = true;
            f[slot].clone_of = use_waffle ? 100 : owner;
            f[slot].shadow_clone = true;
            f[slot].stats = &cs;
            f[slot].x16 = cx * FP;
            f[slot].y16 = op.y16;
            f[slot].facing = op.facing;
            f[slot].hp = cs.max_hp;
            f[slot].clone_timer = CLONE_LIFE_FRAMES;
            f[slot].jumps_left = cs.max_jumps;
            f[slot].on_ground = op.on_ground;
            f[slot].state = op.on_ground ? ST_IDLE : ST_AIR;
            ai[slot] = Ai();

            spawn_effect(FX_SPARK, f[slot].x16, f[slot].y16 - 14 * FP, 12);
            ev.absorb = true;
            ev.dash = true;
        }

        void begin_in_stage_fs(int atk_idx, int vic_idx)
        {
            fs_combo_active = true;
            fs_combo_tick = 0;
            fs_combo_atk = atk_idx;
            fs_combo_vic = vic_idx;
            fs_combo_clone = (team_of(atk_idx) == 0) ? 3 : 4;

            Fighter& atk = f[atk_idx];
            Fighter& vic = f[vic_idx];
            const int mid = stage.length / 2;
            if(atk.x16 / FP <= mid)
            {
                fs_combo_home_x = (mid - 96 >= 28) ? (mid - 96) : 28;
            }
            else
            {
                fs_combo_home_x = (mid + 96 <= stage.length - 28) ? (mid + 96) : (stage.length - 28);
            }

            cancel_move(atk);
            cancel_move(vic);
            atk.vx = 0; atk.vy = 0;
            atk.x16 = fs_combo_home_x * FP;
            atk.y16 = stage.ground_y * FP;
            atk.on_ground = true;
            atk.facing = (fs_combo_home_x < mid) ? 1 : -1;
            atk.state = ST_IDLE;

            vic.vx = 0; vic.vy = 0;
            vic.on_ground = false;
            vic.support = -1;
            vic.state = ST_GRABBED;
            vic.x16 = mid * FP;
            vic.y16 = (stage.ground_y - 44) * FP;

            f[fs_combo_clone] = Fighter();
            f[fs_combo_clone].active = false;
            ev.absorb = true;
        }

        void step_harue_fs()
        {
            ++fs_combo_tick;
            const int t = fs_combo_tick;
            Fighter& atk = f[fs_combo_atk];
            Fighter& cl  = f[fs_combo_clone];
            Fighter& vic = f[fs_combo_vic];
            const int mid = stage.length / 2;
            const int mid_x16 = mid * FP;
            const int air_y16 = (stage.ground_y - 44) * FP;
            const Move* chg_m = atk.stats->moves ? atk.stats->moves->slot[SLOT_SPECIAL] : nullptr;
            const Move* fs_m  = atk.stats->final_smash_move;
            const Move* ud_m  = atk.stats->moves ? atk.stats->moves->slot[SLOT_SPECIAL_UP] : nullptr;

            if(t <= 100)
            {
                vic.x16 = mid_x16;
                vic.y16 = air_y16 + ((t % 16 < 8) ? -FP : FP);
                vic.state = ST_GRABBED;

                if(t < 65)
                {
                    atk.state = ST_IDLE;
                    atk.move = nullptr;
                }
                else if(t < 75)
                {
                    atk.state = ST_ATTACK;
                    atk.move = chg_m;
                    atk.move_frame = 10 + (t - 65);
                }
                else if(t == 75)
                {
                    const Shot boom = Shot().of(PROJ_EXPLOSION).dmg(0).fly(0, 14).size(48, 38)
                                            .steal_proof().lingers();
                    spawn_shot_at(fs_combo_atk, boom, atk.x16, atk.y16 - 14 * FP, 0, false);
                    ev.boom = true;
                    ev.absorb = true;

                    cl = Fighter();
                    cl.active = true;
                    cl.is_clone = true;
                    cl.clone_of = fs_combo_atk;
                    cl.shadow_clone = true;
                    cl.stats = atk.stats;
                    cl.hp = 999;
                    const int sep = (fs_combo_home_x < mid) ? 20 * FP : -20 * FP;
                    cl.x16 = atk.x16 + sep;
                    cl.y16 = atk.y16;
                    cl.facing = atk.facing;
                    cl.state = ST_IDLE;
                    atk.state = ST_IDLE;
                    atk.move = nullptr;
                }
                else
                {
                    atk.state = ST_IDLE;
                    cl.state = ST_IDLE;
                }
            }
            else if(t <= 208)
            {
                const int k = (t - 101) / 18;
                const int sub = (t - 101) % 18;
                const bool left_hit = (k % 2 == 0);
                const int span = (mid - 44 >= 36) ? 44 : (mid - 32);
                const int lx16 = (mid - span) * FP;
                const int rx16 = (mid + span) * FP;

                atk.x16 = lx16 - 16 * FP; atk.y16 = air_y16; atk.facing = 1;
                cl.x16  = rx16 + 16 * FP; cl.y16  = air_y16; cl.facing = -1;

                if(sub < 5)
                {
                    vic.x16 = left_hit ? lx16 : rx16;
                }
                else
                {
                    const int frac = sub - 5;
                    vic.x16 = left_hit ? (lx16 + (rx16 - lx16) * frac / 12)
                                       : (rx16 - (rx16 - lx16) * frac / 12);
                }
                vic.y16 = air_y16 - ((sub >= 5 && sub <= 13) ? 6 * FP : 0);
                vic.state = ST_GRABBED;

                if(left_hit)
                {
                    atk.state = ST_ATTACK; atk.move = fs_m; atk.move_frame = sub;
                    cl.state = ST_IDLE; cl.move = nullptr;
                }
                else
                {
                    cl.state = ST_ATTACK; cl.move = fs_m; cl.move_frame = sub;
                    atk.state = ST_IDLE; atk.move = nullptr;
                }

                if(sub == 5)
                {
                    ev.swing = true;
                    ev.hit = true;
                    if(vic.hp > 5) { vic.hp -= 5; }
                    else if(vic.hp > 1) { vic.hp = 1; }
                    spawn_effect(FX_SPARK, vic.x16, vic.y16 - 14 * FP, 10);
                }
            }
            else if(t <= 232)
            {
                if(t < 217)
                {
                    const int dt = t - 209;
                    const int span = (mid - 44 >= 36) ? 44 : (mid - 32);
                    vic.x16 = (mid - span) * FP + (span * FP * dt / 8);
                    vic.y16 = air_y16;
                    vic.state = ST_GRABBED;
                    atk.x16 = mid_x16 - 16 * FP; atk.y16 = (stage.ground_y - 74) * FP; atk.facing = 1;
                    cl.x16  = mid_x16 + 16 * FP; cl.y16  = (stage.ground_y - 74) * FP; cl.facing = -1;
                    atk.state = ST_AIR; atk.move = nullptr;
                    cl.state  = ST_AIR; cl.move  = nullptr;
                }
                else
                {
                    const int dt = t - 217;
                    const int drop_y16 = (stage.ground_y - 74) * FP + dt * 74 * FP / 15;
                    vic.x16 = mid_x16; vic.y16 = air_y16; vic.state = ST_GRABBED;
                    atk.x16 = mid_x16 - 16 * FP; atk.y16 = drop_y16; atk.facing = 1;
                    cl.x16  = mid_x16 + 16 * FP; cl.y16  = drop_y16; cl.facing = -1;
                    atk.state = ST_ATTACK; atk.move = ud_m; atk.move_frame = 24 + dt;
                    cl.state  = ST_ATTACK; cl.move  = ud_m; cl.move_frame  = 24 + dt;
                    if(t == 217) { ev.swing = true; }
                    if(t == 232)
                    {
                        vic.state = ST_HURT;
                        hit(vic, 26, fp(7.5), fp(-6.5), 42, 1, vic.x16, vic.y16 - 14 * FP, nullptr);
                        const Shot boom = Shot().of(PROJ_EXPLOSION).dmg(0).fly(0, 14).size(56, 44)
                                                .steal_proof().lingers();
                        spawn_shot_at(fs_combo_atk, boom, vic.x16, vic.y16 - 14 * FP, 0, false);
                        ev.boom = true;
                        ev.hitsmash = true;
                        check_winner();
                    }
                }
            }
            else
            {
                integrate(vic, false, false);
                integrate(atk, false, false);
                if(t == 256)
                {
                    spawn_effect(FX_DUST, cl.x16, cl.y16 - 14 * FP, 16);
                    cl.active = false;
                    cl.hp = 0;
                    cancel_move(atk);
                    atk.state = atk.on_ground ? ST_IDLE : ST_AIR;
                    fs_combo_active = false;
                    fs_combo_tick = 0;
                }
            }
            for(Projectile& pr : proj) { if(pr.active && pr.shot.kind == PROJ_EXPLOSION && --pr.life <= 0) { pr.active = false; } }
            for(Effect& e : fx) { if(e.active && ++e.age >= e.life) { e.active = false; } }
            ++frame;
        }

        // ---------------------------------------------------------------------------- blocks & TNT

        void spawn_block(int owner)
        {
            Fighter& p = f[owner];
            if(p.materials < 1) { return; }
            --p.materials;

            int slot = 0;
            for(int k = 0; k < MAX_BLOCKS; ++k)
            {
                if(!blocks[k].active) { slot = k; break; }
                if(blocks[k].id < blocks[slot].id) { slot = k; }
            }
            int bx = p.x16 / FP - BLOCK_W / 2;
            if(bx < BODY_MARGIN) { bx = BODY_MARGIN; }
            if(bx > stage.length - BODY_MARGIN - BLOCK_W) { bx = stage.length - BODY_MARGIN - BLOCK_W; }
            int by = p.y16 / FP;
            if(by < 36) { by = 36; }
            if(by > stage.ground_y - BLOCK_H) { by = stage.ground_y - BLOCK_H; }

            blocks[slot] = PlacedBlock{ true, next_block_id++, owner, bx, by, BLOCK_W, BLOCK_LIFE, BLOCK_HP };
            p.y16 = by * FP;
            p.vy = 0;
            p.on_ground = true;
            p.support = 100 + slot;
        }

        void update_blocks()
        {
            for(PlacedBlock& b : blocks)
            {
                if(b.active && --b.life <= 0) { b.active = false; }
            }
        }

        void spawn_tnt(int owner)
        {
            int tnt_count = 0;
            int oldest_idx = -1;
            for(int k = 0; k < MAX_PROJECTILES; ++k)
            {
                if(proj[k].active && proj[k].shot.kind == PROJ_TNT)
                {
                    ++tnt_count;
                    if(oldest_idx < 0 || proj[k].id < proj[oldest_idx].id) { oldest_idx = k; }
                }
            }
            if(tnt_count >= MAX_TNT && oldest_idx >= 0) { proj[oldest_idx].active = false; }

            const Shot tnt = Shot().of(PROJ_TNT).dmg(16).fly(0, TNT_LIFE).size(18, 18)
                                   .spawn(8, 8).launch(fp(4.5), fp(-4.5)).stun(24).hurts_everyone();
            spawn_shot(owner, tnt, false);
        }

        void detonate_tnt_proj(Projectile& pr)
        {
            const int owner = pr.owner;
            const int x16 = pr.x16;
            const int y16 = pr.y16;
            pr.active = false;
            const Shot boom = Shot().of(PROJ_EXPLOSION).dmg(16).fly(0, 12).size(48, 38)
                                    .launch(fp(4.5), fp(-4.5)).stun(24).steal_proof().lingers().hurts_everyone();
            spawn_shot_at(owner, boom, x16, y16, 0, false);
            ev.boom = true;
        }

        // ---------------------------------------------------------------------------- shots

        // ---------------------------------------------------------------------------- shots

        void spawn_shot(int owner, const Shot& shot, bool apply_buff = true)
        {
            Fighter& p = f[owner];
            if(shot.spread > 0)
            {
                spawn_shot_at(owner, shot, p.x16, p.y16 - shot.dy * FP, -shot.spread, apply_buff, shot.vy);
                spawn_shot_at(owner, shot, p.x16, p.y16 - shot.dy * FP, 0, apply_buff, shot.vy - fp(0.4));
                spawn_shot_at(owner, shot, p.x16, p.y16 - shot.dy * FP, shot.spread, apply_buff, shot.vy);
                return;
            }
            spawn_shot_at(owner, shot, p.x16 + p.facing * shot.dx * FP, p.y16 - shot.dy * FP, p.facing * shot.speed, apply_buff, shot.vy);
        }

        void spawn_shot_at(int owner, const Shot& shot, int x16, int y16, int vx, bool apply_buff = true, int vy_override = 0)
        {
            Fighter& p = f[owner];
            Shot sh = shot;
            if(sh.weapon_scaled)
            {
                sh.damage = scale_weapon_dmg(p, sh.damage);
                sh.kb_x = scale_weapon_kb(p, sh.kb_x);
                sh.kb_y = scale_weapon_kb(p, sh.kb_y);
            }
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
                pr.vy = (vy_override != 0) ? vy_override : sh.vy;
                pr.life = sh.life;
                pr.boom_dir = (vx != 0) ? sign_i(vx) : p.facing;
                pr.shot = sh;
                return;
            }
        }

        void spawn_boom(int owner, int x16, int y16, int damage)
        {
            const Shot boom = Shot().of(PROJ_EXPLOSION).dmg(damage).fly(0, 10).size(40, 30)
                                    .launch(fp(3.5), fp(-4.0)).stun(22).steal_proof().lingers();
            spawn_shot_at(owner, boom, x16, y16, 0);
            ev.boom = true;
        }

        void throw_stored(int i, int power)
        {
            Fighter& p = f[i];
            Shot sh = p.stored;
            if(sh.kind == PROJ_TNT)
            {
                sh.speed = fp(3.0) * power / 100;
                sh.vy = -fp(2.4);
                sh.grav = 3;
                sh.life = 90;
                sh.dx = 12;
                sh.dy = 16;
                sh.boom = sh.damage * power / 100;
                sh.damage = 4;
                sh.friendly_fire = false;
            }
            else
            {
                sh.speed = sh.speed * power / 100;
                sh.damage = sh.damage * power / 100;
                if(sh.damage < 1) { sh.damage = 1; }
            }
            spawn_shot(i, sh, false);
            p.has_stored = false;
            ev.shot = true;
        }

        void update_projectiles()
        {
            for(Projectile& pr : proj)
            {
                if(!pr.active) { continue; }

                // Placed TNT sits in proj[] so Fox Thief / Counter can steal it!
                if(pr.shot.kind == PROJ_TNT && pr.vx == 0)
                {
                    const int old_bot16 = pr.y16 + 8 * FP;
                    pr.vy += 6;
                    if(pr.vy > fp(4.0)) { pr.vy = fp(4.0); }
                    pr.y16 += pr.vy;

                    int best_y16 = stage.ground_y * FP;
                    bool landed = (pr.y16 + 8 * FP) >= best_y16;
                    const int tx = pr.x16 / FP;
                    for(int k = 0; k < stage.platform_count; ++k)
                    {
                        const Platform& pl = stage.platforms[k];
                        const int top16 = pl.y * FP;
                        if(tx >= pl.x && tx <= pl.x + pl.w && old_bot16 <= top16 + 4 * FP &&
                           (pr.y16 + 8 * FP) >= top16 && (!landed || top16 < best_y16))
                        {
                            landed = true;
                            best_y16 = top16;
                        }
                    }
                    for(int k = 0; k < MAX_BLOCKS; ++k)
                    {
                        const PlacedBlock& bl = blocks[k];
                        if(!bl.active) { continue; }
                        const int top16 = bl.y * FP;
                        if(tx >= bl.x && tx <= bl.x + bl.w && old_bot16 <= top16 + 4 * FP &&
                           (pr.y16 + 8 * FP) >= top16 && (!landed || top16 < best_y16))
                        {
                            landed = true;
                            best_y16 = top16;
                        }
                    }
                    if(landed) { pr.y16 = best_y16 - 8 * FP; pr.vy = 0; }

                    ++pr.age;
                    if(--pr.life <= 0) { pr.active = false; continue; }

                    const Rect tbox{ pr.x16 / FP, pr.y16 / FP, pr.shot.w, pr.shot.h };

                    // Check Fox Thief / Counter absorb BEFORE touch detonation!
                    bool stolen = false;
                    for(int ti = 0; ti < MAX_FIGHTERS; ++ti)
                    {
                        if(!f[ti].active || team_of(ti) == team_of(pr.owner) || f[ti].state == ST_KO) { continue; }
                        Fighter& target = f[ti];
                        const Move* tm = target.move;
                        if(!target.has_stored && target.state == ST_ATTACK && tm && tm->absorb_end > tm->absorb_start &&
                           target.move_frame >= tm->absorb_start && target.move_frame < tm->absorb_end)
                        {
                            Rect catch_zone = hurtbox_of(target);
                            catch_zone.w += (ABSORB_MARGIN + 4) * 2;
                            catch_zone.h += (ABSORB_MARGIN + 4) * 2;
                            if(overlaps(tbox, catch_zone))
                            {
                                target.has_stored = true;
                                target.stored = pr.shot;
                                pr.active = false;
                                ev.absorb = true;
                                if(tm->on_absorb)
                            {
                                if(pr.vx != 0) { target.facing = (pr.vx < 0) ? 1 : -1; }
                                else if(f[pr.owner].x16 != target.x16) { target.facing = (f[pr.owner].x16 > target.x16) ? 1 : -1; }
                                begin_move(target, tm->on_absorb, target.move_magic);
                            }
                                stolen = true;
                                break;
                            }
                        }
                    }
                    if(stolen) { continue; }

                    if(pr.age >= TNT_ARM_FRAMES)
                    {
                        bool touched = false;
                        for(int i = 0; i < MAX_FIGHTERS; ++i)
                        {
                            if(!f[i].active || i == pr.owner || f[i].state == ST_KO) { continue; }
                            if(overlaps(tbox, hurtbox_of(f[i]))) { touched = true; break; }
                        }
                        if(!touched)
                        {
                            for(Projectile& other : proj)
                            {
                                if(!other.active || &other == &pr || other.shot.kind == PROJ_EXPLOSION || other.shot.kind == PROJ_TNT) { continue; }
                                if(overlaps(tbox, Rect{ other.x16 / FP, other.y16 / FP, other.shot.w, other.shot.h }))
                                {
                                    if(!other.shot.persist) { other.active = false; }
                                    touched = true;
                                    break;
                                }
                            }
                        }
                        if(touched) { detonate_tnt_proj(pr); }
                    }
                    continue;
                }

                if(pr.shot.boom_accel > 0)
                {
                    const int prev_vx = pr.vx;
                    pr.vx -= pr.boom_dir * pr.shot.boom_accel;
                    if(!pr.boom_flipped && ((prev_vx > 0 && pr.vx <= 0) || (prev_vx < 0 && pr.vx >= 0)))
                    {
                        pr.boom_flipped = true;
                        pr.hit_done = false;
                    }
                    if(pr.boom_flipped)
                    {
                        const Fighter& ow = f[pr.owner];
                        const int target_y16 = ow.y16 - (ow.stats->body_h * FP) / 2;
                        if(pr.y16 < target_y16 - 2 * FP)      { pr.y16 += FP; }
                        else if(pr.y16 > target_y16 + 2 * FP) { pr.y16 -= FP; }
                        if(overlaps(Rect{ pr.x16 / FP, pr.y16 / FP, pr.shot.w, pr.shot.h }, hurtbox_of(ow)))
                        {
                            pr.active = false;
                            continue;
                        }
                    }
                }

                pr.x16 += pr.vx;
                if(pr.shot.grav != 0 || pr.vy != 0)
                {
                    pr.vy += pr.shot.grav;
                    pr.y16 += pr.vy;
                }
                ++pr.age;
                if(--pr.life <= 0 || pr.x16 < -32 * FP || pr.x16 > (stage.length + 32) * FP) { pr.active = false; continue; }

                if(pr.shot.grav != 0 && pr.vy > 0 && pr.y16 >= stage.ground_y * FP)
                {
                    pr.active = false;
                    if(pr.shot.boom > 0) { spawn_boom(pr.owner, pr.x16, stage.ground_y * FP - 8 * FP, pr.shot.boom); }
                    continue;
                }

                if(pr.hit_done) { continue; }

                const Shot& sh = pr.shot;
                const Rect box{ pr.x16 / FP, pr.y16 / FP, sh.w, sh.h };

                if(sh.final_smash == FS_NONE && sh.kind != PROJ_EXPLOSION)
                {
                    bool hit_wall = false;
                    for(int bk = 0; bk < MAX_BLOCKS; ++bk)
                    {
                        PlacedBlock& bl = blocks[bk];
                        if(!bl.active) { continue; }
                        const Rect bl_box{ bl.x + bl.w / 2, bl.y + BLOCK_H / 2, bl.w, BLOCK_H };
                        if(overlaps(box, bl_box))
                        {
                            if(sh.boom > 0) { spawn_boom(pr.owner, pr.x16, pr.y16, sh.boom); }
                            bl.hp -= sh.damage > 1 ? sh.damage : 1;
                            spawn_effect(bl.hp <= 0 ? FX_DUST : FX_SPARK, pr.x16, pr.y16, 10);
                            if(bl.hp <= 0) { bl.active = false; }
                            if(sh.boom_accel > 0 && !pr.boom_flipped)
                            {
                                pr.vx = -pr.vx;
                                pr.boom_flipped = true;
                            }
                            else { pr.active = false; }
                            hit_wall = true;
                            break;
                        }
                    }
                    if(hit_wall) { continue; }
                }

                bool hit_any = false;
                for(int ti = 0; ti < MAX_FIGHTERS && pr.active; ++ti)
                {
                    if(!f[ti].active || f[ti].state == ST_KO) { continue; }
                    const bool same_team = (team_of(ti) == team_of(pr.owner));
                    if(same_team && ti != pr.owner && sh.heal_ally > 0)
                    {
                        if(overlaps(box, hurtbox_of(f[ti])))
                        {
                            f[ti].hp += sh.heal_ally;
                            if(f[ti].hp > f[ti].stats->max_hp) { f[ti].hp = f[ti].stats->max_hp; }
                            spawn_effect(FX_SPARK, pr.x16, pr.y16, 10);
                            ev.absorb = true;
                            pr.active = false;
                            break;
                        }
                    }
                    if(!sh.friendly_fire && same_team) { continue; }

                    Fighter& target = f[ti];
                    Rect body = hurtbox_of(target);

                    const Move* tm = target.move;
                    if(sh.absorbable && sh.kind != PROJ_EXPLOSION && sh.final_smash == FS_NONE &&
                       target.state == ST_ATTACK && tm && tm->reflect_end > tm->reflect_start &&
                       target.move_frame >= tm->reflect_start && target.move_frame < tm->reflect_end)
                    {
                        Rect ref_zone = body;
                        ref_zone.w += ABSORB_MARGIN * 2;
                        ref_zone.h += ABSORB_MARGIN * 2;
                        if(overlaps(box, ref_zone))
                        {
                            pr.owner = ti;
                            pr.vx = (pr.vx != 0) ? -pr.vx : (target.facing * fp(3.5));
                            if(pr.vy > 0) { pr.vy = -pr.vy / 2; }
                            pr.shot.damage = pr.shot.damage * 150 / 100;
                            pr.life = sh.life;
                            pr.hit_done = false;
                            spawn_effect(FX_SPARK, pr.x16, pr.y16, 10);
                            ev.absorb = true;
                            break;
                        }
                    }

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
                            if(tm->on_absorb) { begin_move(target, tm->on_absorb, target.move_magic); }
                            break;
                        }
                    }

                    if(overlaps(box, body))
                    {
                        int dir = sign_i(pr.vx);
                        if(dir == 0) { dir = sign_i(target.x16 - pr.x16); }
                        if(dir == 0) { dir = f[pr.owner].facing; }

                        if(sh.final_smash != FS_NONE && !target.is_clone)
                        {
                            if(target.state == ST_SHIELD) { target.state = ST_IDLE; }
                            target.on_pig = false;
                            target.pig_hp = 0;
                            int applied = sh.damage;
                            if(target.hp <= applied) { applied = target.hp > 1 ? target.hp - 1 : 0; }
                            hit(target, applied, sh.kb_x, sh.kb_y, sh.hitstun, dir, pr.x16, pr.y16, nullptr);
                            pr.active = false;
                            if(pending_final_smash < 0)
                            {
                                pending_final_smash = pr.owner;
                                pending_final_smash_victim = ti;
                                pending_final_smash_id = sh.final_smash;
                            }
                            return;
                        }

                        hit(target, sh.damage, sh.kb_x, sh.kb_y, sh.hitstun, dir, pr.x16, pr.y16,
                            sh.friendly_fire ? nullptr : &f[pr.owner]);
                        if(sh.weapon_scaled) { use_weapon_durability(f[pr.owner]); }
                        if(sh.boom > 0) { spawn_boom(pr.owner, pr.x16, pr.y16, sh.boom); }
                        hit_any = true;
                        if(!sh.persist) { pr.active = false; break; }
                    }
                }
                if(hit_any && sh.persist) { pr.hit_done = true; }
            }
        }

        // ---------------------------------------------------------------------------- melee

        // Checks every active hitbox against the other team. Everything that touches this frame is collected first and
        // applied afterwards, so two fighters that hit each other on the same frame BOTH get hit (a trade).
        void resolve_melee()
        {
            PendingHit pending[MAX_FIGHTERS * MAX_HITBOXES * 2];
            int pending_count = 0;

            for(int a = 0; a < MAX_FIGHTERS; ++a)
            {
                const Fighter& p = f[a];
                if(!p.active || p.state != ST_ATTACK || !p.move) { continue; }

                for(int v = 0; v < MAX_FIGHTERS; ++v)
                {
                    const Fighter& t = f[v];
                    if(!t.active || a == v || t.state == ST_KO) { continue; }
                    const bool same_team = (team_of(v) == team_of(a));

                    const Rect body = hurtbox_of(t);
                    for(int k = 0; k < p.move->hit_count; ++k)
                    {
                        const Hitbox& h = p.move->hits[k];
                        if(same_team && !h.hits_ally) { continue; }
                        Rect box;
                        if(!hitbox_now(a, k, box) || (p.hit_done & (1u << k)) || !overlaps(box, body)) { continue; }

                        const int gap = t.x16 - p.x16;
                        PendingHit& ph = pending[pending_count++];
                        ph.attacker = a;
                        ph.victim = v;
                        ph.index = k;
                        ph.box = h;
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
                if(attacker.state == ST_GRABBED) { continue; }
                if(ph.stops_dash && attacker.state == ST_ATTACK) { attacker.drive_cut = true; }

                if(ph.box.grab) { grab_connects(ph.attacker, ph.victim); continue; }

                const bool same_team = (team_of(ph.victim) == team_of(ph.attacker));
                int fs_id = (attacker.move && attacker.move->final_smash != FS_NONE)
                                  ? ((same_team && attacker.move->ally_final_smash != FS_NONE)
                                     ? attacker.move->ally_final_smash : attacker.move->final_smash)
                                  : FS_NONE;
                if(fs_id != FS_NONE && victim.is_clone)
                {
                    fs_id = FS_NONE;
                    attacker.hit_done &= ~(1u << ph.index);  // slice through clone without wasting the FS swing
                }

                if(same_team && fs_id != FS_NONE)
                {
                    if(pending_final_smash < 0)
                    {
                        pending_final_smash = ph.attacker;
                        pending_final_smash_victim = ph.victim;
                        pending_final_smash_id = fs_id;
                    }
                    continue;
                }
                if(same_team) { continue; }

                const Move* vm = victim.move;
                if(fs_id == FS_NONE && vm && vm->on_counter_melee && in_window(victim, vm->counter_start, vm->counter_end))
                {
                    counter_melee(ph.victim, ph.attacker);
                    continue;
                }

                const int base_dmg = ph.box.weapon_scaled ? scale_weapon_dmg(attacker, ph.box.damage) : ph.box.damage;
                const int kb_x = ph.box.weapon_scaled ? scale_weapon_kb(attacker, ph.box.kb_x) : ph.box.kb_x;
                const int kb_y = ph.box.weapon_scaled ? scale_weapon_kb(attacker, ph.box.kb_y) : ph.box.kb_y;
                const int damage = buffed_damage(attacker, base_dmg);

                if(ph.box.keep_grab && victim.grabbed_by == ph.attacker)
                {
                    scratch(victim, damage, kb_x, kb_y, ph.dir, fs_id != FS_NONE ? nullptr : &attacker);
                    spawn_effect(FX_SPARK, ph.sx16, ph.sy16, 10);
                    continue;
                }

                int applied_dmg = damage;
                if(fs_id != FS_NONE)
                {
                    if(victim.state == ST_SHIELD) { victim.state = ST_IDLE; }
                    if(victim.hp <= applied_dmg) { applied_dmg = victim.hp > 1 ? victim.hp - 1 : 0; }
                }

                hit(victim, applied_dmg, kb_x, kb_y, ph.box.hitstun, ph.dir, ph.sx16, ph.sy16, fs_id != FS_NONE ? nullptr : &attacker);
                if(ph.box.weapon_scaled && fs_id == FS_NONE) { use_weapon_durability(attacker); }
                if(ph.box.delayed_boom_dmg > 0 && attacker.har_power_timer > 0 && victim.hp > 0)
                {
                    victim.bomb_timer = HAR_BOMB_DELAY;
                    victim.bomb_owner = ph.attacker;
                    victim.bomb_dmg = ph.box.delayed_boom_dmg;
                }
                if(fs_id != FS_NONE)
                {
                    if(attacker.move && attacker.move->powered_fs != FS_NONE && attacker.har_power_timer > 0)
                    {
                        const int pfs_id = attacker.move->powered_fs;
                        attacker.har_power_timer = 0;
                        if(pending_final_smash < 0)
                        {
                            pending_final_smash = ph.attacker;
                            pending_final_smash_victim = ph.victim;
                            pending_final_smash_id = pfs_id;
                        }
                        return;
                    }
                    if(attacker.move && attacker.move->in_stage_fs)
                    {
                        begin_in_stage_fs(ph.attacker, ph.victim);
                        return;
                    }
                    if(pending_final_smash < 0)
                    {
                        pending_final_smash = ph.attacker;
                        pending_final_smash_victim = ph.victim;
                        pending_final_smash_id = fs_id;
                    }
                }
                if(ph.box.pogo > 0) { pogo(attacker, ph.box.pogo); }
            }

            // Melee attacks can chip/break placed wall blocks
            for(int a = 0; a < MAX_FIGHTERS; ++a)
            {
                Fighter& p = f[a];
                if(!p.active || p.state != ST_ATTACK || !p.move) { continue; }
                for(int k = 0; k < p.move->hit_count; ++k)
                {
                    Rect box;
                    if(!hitbox_now(a, k, box) || (p.hit_done & (1u << k))) { continue; }
                    for(int bk = 0; bk < MAX_BLOCKS; ++bk)
                    {
                        PlacedBlock& bl = blocks[bk];
                        if(!bl.active) { continue; }
                        const Rect bl_box{ bl.x + bl.w / 2, bl.y + BLOCK_H / 2, bl.w, BLOCK_H };
                        if(overlaps(box, bl_box))
                        {
                            p.hit_done |= 1u << k;
                            const int d = p.move->hits[k].damage;
                            bl.hp -= d > 2 ? d : 2;
                            spawn_effect(bl.hp <= 0 ? FX_DUST : FX_SPARK, (bl.x + bl.w / 2) * FP, (bl.y + BLOCK_H / 2) * FP, 10);
                            if(bl.hp <= 0) { bl.active = false; }
                            ev.hit = true;
                            break;
                        }
                    }
                }
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
            v.on_pig = false;
            v.pig_hp = 0;
            v.pig_hits = 0;
            v.pig_hit_cd = 0;
            v.state = ST_GRABBED;
            v.grabbed_by = grabber;
            v.vx = 0;
            v.vy = 0;
            v.timer = 0;
            g.holding = victim;
            g.vx = 0;
            g.vy = 0;
            ev.grab = true;
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
            for(int v = 0; v < MAX_FIGHTERS; ++v)
            {
                Fighter& vf = f[v];
                if(!vf.active) { continue; }
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
            ev.grab = true;
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
            if(damage <= 0) { return damage; }
            if(attacker.har_power_timer > 0) { damage = damage * 130 / 100; }                            // (a hit that only stuns stays a hit that only stuns)
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
            if(t.state == ST_ATTACK && t.move && t.move->final_smash != FS_NONE) { return; }
            const FighterStats& s = *t.stats;
            clear_grab_of(t);

            if(t.on_pig)
            {
                const int pig_dmg = damage > 0 ? damage : 1;
                t.pig_hp -= pig_dmg;
                if(t.pig_hp <= 0)
                {
                    t.on_pig = false;
                    t.pig_hp = 0;
                    t.pig_hits = 0;
                    t.pig_hit_cd = 0;
                    spawn_effect(FX_DUST, t.x16 - 6 * FP, t.y16 - 8 * FP, 18);
                    spawn_effect(FX_DUST, t.x16 + 6 * FP, t.y16 - 10 * FP, 20);
                    cancel_move(t);
                    t.state = ST_HURT;
                    t.timer = 24;
                    launch(t, fp(2.0), fp(-3.8), dir);
                    ev.hitsmash = true;
                }
                else
                {
                    ev.hit = true;
                }
                return;
            }
            int reduction = s.defense + t.def_buff_amount;
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
                if(t.is_clone)
                {
                    if(net_client)
                    {
                        t.hp = 1;
                        t.state = ST_HURT;
                        t.timer = 8;
                    }
                    else
                    {
                        t.active = false;
                        spawn_effect(FX_DUST, t.x16, t.y16 - 12 * FP, 16);
                    }
                }
                else
                {
                    launch(t, kb_x, kb_y, dir);
                    ev.ko = true;
                }
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
