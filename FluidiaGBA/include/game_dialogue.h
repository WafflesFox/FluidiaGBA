#ifndef GAME_DIALOGUE_H
#define GAME_DIALOGUE_H

// ================================================================================================
// The dialogue box (Conversation)
//
// Part of main.cpp. Include it ONLY from main.cpp, after main.cpp's own #includes and in the order
// listed there: it uses everything main.cpp has declared before it.
// ================================================================================================

namespace
{
    // ================================================================================
    // THE DIALOGUE BOX
    // ================================================================================

    struct Conversation
    {
        Npc* npc = nullptr;                          // who we are talking to; nullptr = nobody
        int node = 0;                                // which DialogueNode of theirs is on screen
        int choice = 0;                              // which answer is highlighted (when the node asks something)
        int battle_npc = -1;                         // set by an ACT_BATTLE line: the NPC to fight once the talk is over (-1 = none)
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
        if(node.action == ACT_BATTLE && npc.def->fight)
        {
            c.battle_npc = npc.id;                    // the fight begins when the player closes this line (see game_screen)
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

        for(int i = 0; i < 4; ++i)                   // a black box behind the text (reuses transition sprite)
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
                sfx(2);
                --c.choice;
                draw_choices(c, text);
            }
            else if(bn::keypad::right_pressed() && c.choice < node.choice_count - 1)
            {
                sfx(2);
                ++c.choice;
                draw_choices(c, text);
            }

            if(bn::keypad::a_pressed())
            {
                sfx(1);
                next = node.choices[c.choice].next;
                advance = true;
            }
        }
        else if(bn::keypad::a_pressed())
        {
            sfx(1);
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
}

#endif
