#ifndef GAME_NPC_DATA_H
#define GAME_NPC_DATA_H

// ================================================================================================
// NPC data: shared helpers, traits/buffs, dialogue nodes and every NPC's dialogue
//
// Part of main.cpp. Include it ONLY from main.cpp, after main.cpp's own #includes and in the order
// listed there: it uses everything main.cpp has declared before it.
// ================================================================================================

namespace
{
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

    enum { ACT_NONE, ACT_FOLLOW, ACT_STOP_FOLLOW, ACT_BATTLE};   // things a dialogue node can make the NPC do (ACT_BATTLE: fight starts when the talk ends)

    enum //traits
    {
        TRAIT_LAZY      = 1 << 0,
        TRAIT_SPEEDY    = 1 << 1,
        TRAIT_INTROVERT = 1 << 2,
        TRAIT_EXTROVERT = 1 << 3,
        TRAIT_WALKER    = 1 << 4,
        TRAIT_PLAYFUL   = 1 << 5,
    };

    // ---- BATTLE BUFFS ----
    // A follower WITHOUT fighting data can't join a battle, but it stands by and gives you a small bonus from its traits.
    // All numbers are percents; a follower with two traits gives both. Tune them here, nothing else has to change.
    constexpr int BUFF_LAZY_PHYS       = 20;   // Lazy:      your A moves (slaps, smashes, aerials) hit 20% harder
    constexpr int BUFF_SPEEDY_SPEED    = 15;   // Speedy:    you walk 15% faster
    constexpr int BUFF_INTROVERT_DEF   = 10;   // Introvert: 10% of all damage you take is removed (on top of your own defense)
    constexpr int BUFF_EXTROVERT_MAGIC = 20;   // Extrovert: your B moves (water gun, aqua jets) hit 20% harder
    constexpr int BUFF_WALKER_JUMP     = 6;    // Walker:    you jump 6% faster off the ground (about 12% higher)
    constexpr int BUFF_PLAYFUL_DAMAGE  = 10;   // Playful:   a little more of BOTH A and B damage

    battle::Buff buff_from_traits(int traits)
    {
        battle::Buff b;
        if(traits & TRAIT_LAZY)      { b.phys_pct  += BUFF_LAZY_PHYS; }
        if(traits & TRAIT_SPEEDY)    { b.speed_pct += BUFF_SPEEDY_SPEED; }
        if(traits & TRAIT_INTROVERT) { b.defense   += BUFF_INTROVERT_DEF; }
        if(traits & TRAIT_EXTROVERT) { b.magic_pct += BUFF_EXTROVERT_MAGIC; }
        if(traits & TRAIT_WALKER)    { b.jump_pct  += BUFF_WALKER_JUMP; }
        if(traits & TRAIT_PLAYFUL)   { b.phys_pct  += BUFF_PLAYFUL_DAMAGE; b.magic_pct += BUFF_PLAYFUL_DAMAGE; }
        return b;
    }

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
        const battle::FighterDef* fight = nullptr;   // fighting data. nullptr = this NPC can not fight (never give one a battle dialogue)
    };

    // ---- DIALOGUES ----
    // Give every node a name here so the tables below stay readable. The ORDER must match snep_nodes[].
    enum SnepNode
    {
        SNEP_BABE, SNEP_TRAPPED, SNEP_LATER, SNEP_ALONE, SNEP_DOTS, SNEP_UPLOAD, SNEP_FOOTAGE,
        SNEP_OKAY, SNEP_OKAY_YES, SNEP_OKAY_NO,
        SNEP_MENU,
        SNEP_FOLLOW_Q, SNEP_FOLLOW_YES, SNEP_CONCERN,SNEP_FOLLOW_NO,
        SNEP_STOP_Q, SNEP_STOP_YES, SNEP_STOP_NO,
        SNEP_FIGHT,
    };

    enum FoxNode
    {
        FOX_CLONE, FOX_WOAH, FOX_CONFUSE,
        FOX_MENU, FOX_YES, FOX_NO,
        FOX_STOP, FOX_STOP_YES, FOX_STOP_NO,
        FOX_HUB, FOX_FIGHT_Q, FOX_FIGHT_YES, FOX_FIGHT_NO,
    };

    enum UmbNode
    {
        UMB_START, UMB_GREET, UMB_MAGICAL,
        UMB_REST,
        UMB_RECOMMEND, UMB_RECOMMEND_2, UMB_YES, UMB_NO,
        UMB_STOP_Q, UMB_STOP_Y, UMB_STOP_N,
        UMB_RECOMMEND3,
        UMB_FIGHT,
    };
    enum HarNode
    {
        HAR_START, HAR_EXCITE, HAR_GOOD,
        HAR_FOLLOW, HAR_YES, HAR_NO,
        HAR_FOLLOW_Q, HAR_STOP_Y, HAR_STOP_N,
        HAR_FIGHT,
    };

    constexpr Choice snep_okay_choices[]   = { { "Yes", SNEP_OKAY_YES }, { "Not really", SNEP_OKAY_NO } };
    constexpr Choice snep_menu_choices[]   = { { "Follow me", SNEP_FOLLOW_Q }, {"Fight",SNEP_FIGHT},{ "Nothing", DLG_END } };
    constexpr Choice snep_follow_choices[] = { { "Yes", SNEP_FOLLOW_YES }, { "No", SNEP_FOLLOW_NO } };
    constexpr Choice snep_stop_choices[]   = { { "Wait here", SNEP_STOP_YES }, { "Keep going", SNEP_STOP_NO } };
    
    constexpr Choice fox_follow_choices[] = {{"Yes", FOX_YES}, {"No", FOX_NO}};
    constexpr Choice fox_stop_choices[] = {{"Yes", FOX_STOP_YES}, {"No", FOX_STOP_NO}};
    constexpr Choice fox_hub_choices[] = {{"Follow", FOX_MENU}, {"Fight", FOX_FIGHT_Q}, {"Nothing", DLG_END}};
    constexpr Choice fox_fight_choices[] = {{"Lets try", FOX_FIGHT_YES}, {"Nope", FOX_FIGHT_NO}};

    constexpr Choice umb_follow_choices[] = {{"Yes", UMB_YES}, {"No", UMB_NO}, {"Fight", UMB_FIGHT}};
    constexpr Choice umb_stop_choices[] = {{"Right here", UMB_STOP_Y}, {"Not yet", UMB_STOP_N}, {"Fight", UMB_FIGHT}};

    constexpr Choice har_follow_choices[] = {{"Yes, Dad", HAR_YES}, {"Fight!", HAR_FIGHT}, {"Nah", HAR_NO}};
    constexpr Choice har_stop_choices[] = {{"Yeah..", HAR_STOP_Y}, {"Fight!", HAR_FIGHT}, {"Not yet.", HAR_STOP_N}};

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
                              say("Aight, lets train :3", DLG_END, ACT_BATTLE),
    };

    constexpr DialogueNode fox_nodes[]=
    {
        say("Woah", FOX_WOAH),
        say("You're Me! I'm You!", FOX_CONFUSE),
        say("Well Hi there cutie!", DLG_END, ACT_NONE, FOX_HUB),
        ask("Huh, want me to follow you?", fox_follow_choices),
        say("Oh, Okay. Lets go :3", DLG_END, ACT_FOLLOW, FOX_STOP ),
        say("Oh, uhm okay then,"),
        ask("Want me to stop following you?",fox_stop_choices),
        say("Okay then, Unfollowed!", DLG_END, ACT_STOP_FOLLOW, FOX_HUB),
        say("Okay, lets gooo uwu!"),
        ask("What's up, me?", fox_hub_choices),                          // FOX_HUB: talking to him now starts here
        ask("Fight? Me versus me!", fox_fight_choices),                  // FOX_FIGHT_Q
        say("Im gonna wet you. Literally uwu", DLG_END, ACT_BATTLE),     // FOX_FIGHT_YES: the battle starts when this line is closed
        say("Aww... maybe later."),                                      // FOX_FIGHT_NO
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
        say("Alright, let's battle owo!", DLG_END, ACT_BATTLE),
    };

    constexpr DialogueNode harue_nodes[]=
    {
        say("Heyya Son, Good day!", HAR_EXCITE),
        say("I cant wait to fight soon.", HAR_GOOD),
        say("Just polishing my ol weapon yknow?", DLG_END, ACT_NONE, HAR_FOLLOW),
        ask("Huh, want to show me something?", har_follow_choices),
        say("Oh, lead the way son!", DLG_END, ACT_FOLLOW, HAR_FOLLOW_Q),
        say("Goodluck out there"),
        ask("Is that all, Son?", har_stop_choices),
        say("That was fun.", DLG_END, ACT_STOP_FOLLOW, HAR_FOLLOW),
        say("Okie dokie then. Lead the way!"),
        say("Alright son, show me what you got!", DLG_END, ACT_BATTLE),
    };
}

#endif
