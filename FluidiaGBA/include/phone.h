#ifndef PHONE_H
#define PHONE_H

// ================================================================================================
// THE PHONE: the part that DRAWS and reads buttons. The rules of the apps live in phone_logic.h.
// Everything on screen is sprites, so keep an eye on the 128-sprite limit (see LOG_LINES below).
// ================================================================================================

#include "bn_fixed.h"
#include "bn_keypad.h"
#include "bn_optional.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "bn_sprite_items_phone_body.h"
#include "bn_sprite_items_phone_icons.h"
#include "bn_sprite_items_phone_cursor.h"
#include "bn_sprite_items_phone_fill.h"
#include "bn_sprite_items_flappy_bird.h"
#include "bn_sprite_items_flappy_pipe.h"

#include "phone_logic.h"
#include "changelog.h"

namespace phone
{
    // ---- where things are (screen coordinates: 0,0 is the middle, y goes DOWN) ----
    constexpr int PHONE_LEFT = -8;                 // the phone is 128 x 160 pixels and sits on the right
    constexpr int SCREEN_L   = 0;                  // the phone's own screen: x from 0 to 112, y from -66 to 66
    constexpr int SCREEN_R   = 112;
    constexpr int SCREEN_CX  = 56;

    // ---- the calculator keypad ----
    constexpr int KEY_ROWS = 5;
    constexpr int KEY_COLS = 4;
    constexpr char KEYS[KEY_ROWS][KEY_COLS] =
    {
        { 'C', '<', 'N', '/' },
        { '7', '8', '9', '*' },
        { '4', '5', '6', '-' },
        { '1', '2', '3', '+' },
        { '0', 'Z', '.', '=' },
    };

    inline const char* key_label(char k)
    {
        static char single[2] = { 0, 0 };
        if(k == 'N') { return "+/-"; }
        if(k == 'Z') { return "00"; }
        single[0] = k;                              // the text generator reads it at once, so one shared buffer is fine
        return single;
    }

    inline int key_x(int col) { return SCREEN_L + 14 + col * 28; }
    inline int key_y(int row) { return -18 + row * 18; }

    // ---- the changelog ----
    constexpr int LOG_WIDTH = 13;                  // characters per line (must be 15 or less)
    constexpr int LOG_LINES = 5;                   // lines shown at once: each character is a SPRITE, so don't go wild
    constexpr int LOG_MAX   = 96;                  // total wrapped lines kept in memory

    enum App { APP_HOME, APP_CALC, APP_FLAPPY, APP_LOG, APP_STOPWATCH };

    constexpr int APP_COUNT = 4;
    constexpr int APP_IDS[APP_COUNT] = { APP_CALC, APP_FLAPPY, APP_LOG, APP_STOPWATCH };   // the order on the home screen
    constexpr const char* APP_NAMES[APP_COUNT] = { "Calc", "Bird", "Log", "Time" };

    inline int home_x(int i) { return SCREEN_CX + (i % 3 - 1) * 36; }     // 3 icons per row
    inline int home_y(int i) { return -16 + (i / 3) * 44; }                // the label is 24 below the icon

    class Phone
    {
    public:
        bool active() const { return is_open; }

        void open(bn::sprite_text_generator& text)
        {
            is_open = true;
            go_home(text);
        }

        // call once per frame while the phone is open
        void update(bn::sprite_text_generator& text)
        {
            ++ticks;
            switch(app)
            {
                case APP_CALC:    update_calc(text);    break;
                case APP_FLAPPY:  update_flappy(text);  break;
                case APP_LOG:     update_log(text);     break;
                case APP_STOPWATCH: update_stopwatch(text); break;
                default:          update_home(text);    break;
            }
        }

    private:
        bool is_open = false;
        int app = APP_HOME;
        unsigned ticks = 0;

        // everything on screen is kept in these, so clearing a vector removes those sprites
        bn::vector<bn::sprite_ptr, 20> body;       // the phone itself
        bn::vector<bn::sprite_ptr, 40> ui;         // labels and icons that stay put
        bn::vector<bn::sprite_ptr, 80> display;    // text that changes (calculator display, score, changelog lines)
        bn::optional<bn::sprite_ptr> cursor;       // the highlight rectangle

        int home_sel = 0;

        Calculator calc;
        int calc_row = 2;
        int calc_col = 1;

        Stopwatch stopwatch;
        int shown_tenths = -1;

        bn::vector<TextLine, LOG_MAX> log_lines;
        int log_scroll = 0;

        FlappyBird bird;
        bn::vector<bn::sprite_ptr, 12> fill;       // sky backdrop for the full-screen game
        bn::vector<bn::sprite_ptr, 18> pipes;      // 3 pipes x (3 pieces above the gap + 3 below)
        bn::optional<bn::sprite_ptr> bird_sprite;
        int bird_frame = -1;
        int shown_score = -1;
        int shown_state = -1;

        // ---------------------------------------------------------------- shared helpers

        void clear_screen()
        {
            ui.clear();
            display.clear();
            cursor.reset();
        }

        void clear_flappy()
        {
            fill.clear();
            pipes.clear();
            bird_sprite.reset();
        }

        void make_body()
        {
            body.clear();
            for(int r = 0; r < 5; ++r)
            {
                for(int c = 0; c < 4; ++c)
                {
                    bn::sprite_ptr s = bn::sprite_items::phone_body.create_sprite(
                            bn::fixed(PHONE_LEFT + 16 + c * 32), bn::fixed(-64 + r * 32), r * 4 + c);
                    s.set_bg_priority(0);          // in front of the map, roofs included
                    body.push_back(s);
                }
            }
        }

        void place_cursor(int x, int y)
        {
            if(!cursor)
            {
                cursor = bn::sprite_items::phone_cursor.create_sprite(bn::fixed(x), bn::fixed(y));
                cursor->set_bg_priority(0);
            }
            else
            {
                cursor->set_position(bn::fixed(x), bn::fixed(y));
            }
        }

        void close()
        {
            is_open = false;
            body.clear();
            clear_screen();
            clear_flappy();
        }

        // ---------------------------------------------------------------- HOME

        void go_home(bn::sprite_text_generator& text)
        {
            app = APP_HOME;
            clear_flappy();
            clear_screen();
            make_body();

            text.generate(SCREEN_CX, -52, "PHONE", ui);
            for(int i = 0; i < APP_COUNT; ++i)
            {
                bn::sprite_ptr icon = bn::sprite_items::phone_icons.create_sprite(
                bn::fixed(home_x(i)), bn::fixed(home_y(i)), i);
                icon.set_bg_priority(0);
                ui.push_back(icon);
                text.generate(home_x(i), home_y(i) + 24, APP_NAMES[i], ui);
            }
            place_cursor(home_x(home_sel), home_y(home_sel) + 24);
        }

        void open_app(int id, bn::sprite_text_generator& text)
        {
            switch(id)
            {
                case APP_CALC:       open_calc(text);       break;
                case APP_FLAPPY:     open_flappy(text);     break;
                case APP_LOG:        open_log(text);        break;
                case APP_STOPWATCH:  open_stopwatch(text);  break;
                default:                                    break;
            }
        }

        void update_home(bn::sprite_text_generator& text)
        {
            int next = home_sel;
            if(bn::keypad::left_pressed() && home_sel % 3 > 0)                                   { next = home_sel - 1; }
            else if(bn::keypad::right_pressed() && home_sel % 3 < 2 && home_sel + 1 < APP_COUNT) { next = home_sel + 1; }
            else if(bn::keypad::up_pressed() && home_sel >= 3)                                   { next = home_sel - 3; }
            else if(bn::keypad::down_pressed() && home_sel + 3 < APP_COUNT)                      { next = home_sel + 3; }

            if(next != home_sel)
            {
                home_sel = next;
                place_cursor(home_x(home_sel), home_y(home_sel) + 24);
            }

            if(bn::keypad::a_pressed())                                   { open_app(APP_IDS[home_sel], text); }
            else if(bn::keypad::b_pressed() || bn::keypad::l_pressed())   { close(); }
        }

        // ---------------------------------------------------------------- CALCULATOR

        void open_calc(bn::sprite_text_generator& text)
        {
            app = APP_CALC;
            clear_screen();
            calc.clear();

            for(int r = 0; r < KEY_ROWS; ++r)
            {
                for(int c = 0; c < KEY_COLS; ++c)
                {
                    text.generate(key_x(c), key_y(r), key_label(KEYS[r][c]), ui);
                }
            }
            place_cursor(key_x(calc_col), key_y(calc_row));
            refresh_calc(text);
        }

        void refresh_calc(bn::sprite_text_generator& text)
        {
            char small_line[20];
            char big_line[20];
            calc.op_line(small_line);
            calc.main_line(big_line);

            display.clear();
            text.set_right_alignment();                     // numbers line up on the right edge
            text.generate(SCREEN_R - 4, -56, small_line, display);
            text.generate(SCREEN_R - 4, -38, big_line, display);
            text.set_center_alignment();                    // the rest of the game expects centered text
        }

        void update_calc(bn::sprite_text_generator& text)
        {
            bool moved = false;
            if(bn::keypad::left_pressed())       { calc_col = (calc_col + KEY_COLS - 1) % KEY_COLS; moved = true; }
            else if(bn::keypad::right_pressed()) { calc_col = (calc_col + 1) % KEY_COLS;            moved = true; }
            else if(bn::keypad::up_pressed())    { calc_row = (calc_row + KEY_ROWS - 1) % KEY_ROWS; moved = true; }
            else if(bn::keypad::down_pressed())  { calc_row = (calc_row + 1) % KEY_ROWS;            moved = true; }

            if(moved) { place_cursor(key_x(calc_col), key_y(calc_row)); }

            if(bn::keypad::a_pressed())
            {
                calc.press(KEYS[calc_row][calc_col]);
                refresh_calc(text);
            }
            else if(bn::keypad::b_pressed())
            {
                go_home(text);
            }
        }
        void open_stopwatch(bn::sprite_text_generator& text)
        {
            app = APP_STOPWATCH;
            clear_screen();                                          // always start by wiping the previous screen
            text.generate(SCREEN_CX, -56, "STOPWATCH", ui);          // text that never changes goes in ui
            text.generate(SCREEN_CX, 30, "A: start/stop", ui);
            text.generate(SCREEN_CX, 48, "DOWN: reset", ui);
            refresh_stopwatch(text);
        }

        void refresh_stopwatch(bn::sprite_text_generator& text)
        {
            display.clear();                                         // text that changes goes in display
            char time_text[12];
            stopwatch.format(time_text);
            text.generate(SCREEN_CX, -10, time_text, display);
            shown_tenths = stopwatch.frames / 6;
        }

        void update_stopwatch(bn::sprite_text_generator& text)
        {
            stopwatch.update();                                      // the rules count the frames
            if(bn::keypad::a_pressed())         { stopwatch.toggle(); }
            else if(bn::keypad::down_pressed()) { stopwatch.reset(); }
            else if(bn::keypad::b_pressed())    { go_home(text); return; }   // B always goes back

            if(stopwatch.frames / 6 != shown_tenths) { refresh_stopwatch(text); }   // redraw only when the digit changes
        }

        // ---------------------------------------------------------------- CHANGELOG

        void open_log(bn::sprite_text_generator& text)
        {
            app = APP_LOG;
            clear_screen();

            log_lines.clear();                              // break every entry of changelog.h into short lines
            for(int e = 0; e < changelog_count; ++e)
            {
                TextLine wrapped[8];
                const int n = wrap_text(changelog_entries[e], LOG_WIDTH, wrapped, 8);
                for(int i = 0; i < n && !log_lines.full(); ++i) { log_lines.push_back(wrapped[i]); }
                if(!log_lines.full())                       // an empty line between entries
                {
                    TextLine blank;
                    blank.text[0] = 0;
                    log_lines.push_back(blank);
                }
            }

            log_scroll = 0;
            text.generate(SCREEN_CX, -56, "CHANGELOG", ui);
            refresh_log(text);
        }

        int log_max_scroll() const
        {
            const int extra = log_lines.size() - LOG_LINES;
            return extra > 0 ? extra : 0;
        }

        void refresh_log(bn::sprite_text_generator& text)
        {
            display.clear();
            text.set_left_alignment();
            for(int i = 0; i < LOG_LINES; ++i)
            {
                const int index = log_scroll + i;
                if(index < log_lines.size() && log_lines[index].text[0] != 0)
                {
                    text.generate(SCREEN_L + 6, -36 + i * 18, log_lines[index].text, display);
                }
            }
            text.set_center_alignment();

            if(log_scroll > 0)               { text.generate(SCREEN_R - 8, -36, "^", display); }
            if(log_scroll < log_max_scroll()) { text.generate(SCREEN_R - 8, 48, "v", display); }
        }

        void update_log(bn::sprite_text_generator& text)
        {
            if(bn::keypad::up_pressed() && log_scroll > 0)
            {
                --log_scroll;
                refresh_log(text);
            }
            else if(bn::keypad::down_pressed() && log_scroll < log_max_scroll())
            {
                ++log_scroll;
                refresh_log(text);
            }
            else if(bn::keypad::b_pressed())
            {
                go_home(text);
            }
        }

        // ---------------------------------------------------------------- FLAPPY BIRD (full screen)

        void open_flappy(bn::sprite_text_generator& text)
        {
            app = APP_FLAPPY;
            body.clear();                                   // the phone folds away: this game uses the whole screen
            clear_screen();
            clear_flappy();

            for(int r = 0; r < 3; ++r)
            {
                for(int c = 0; c < 4; ++c)
                {
                    bn::sprite_ptr s = bn::sprite_items::phone_fill.create_sprite(bn::fixed(-96 + c * 64), bn::fixed(-64 + r * 64));
                    s.set_bg_priority(0);
                    fill.push_back(s);
                }
            }

            // 6 pieces per pipe: pieces 0-2 go above the opening, 3-5 below. Piece 0 / 3 have the cap.
            for(int i = 0; i < FlappyBird::PIPES; ++i)
            {
                for(int k = 0; k < 6; ++k)
                {
                    int frame = 0;                          // 0 = plain pipe, 1 = cap at the bottom, 2 = cap at the top
                    if(k == 0) { frame = 1; }
                    if(k == 3) { frame = 2; }
                    bn::sprite_ptr s = bn::sprite_items::flappy_pipe.create_sprite(bn::fixed(0), bn::fixed(0), frame);
                    s.set_bg_priority(0);
                    s.set_visible(false);
                    pipes.push_back(s);
                }
            }

            bird_sprite = bn::sprite_items::flappy_bird.create_sprite(bn::fixed(FlappyBird::BIRD_X), bn::fixed(0), 0);
            bird_sprite->set_bg_priority(0);
            bird_frame = 0;

            bird.reset(ticks + 1);
            shown_score = -1;
            shown_state = -1;
            refresh_flappy_text(text);
        }

        void refresh_flappy_text(bn::sprite_text_generator& text)
        {
            display.clear();
            shown_score = bird.score;
            shown_state = bird.state;

            char number[16];
            format_int(bird.score, number);
            text.generate(0, -66, number, display);

            if(bird.state == FlappyBird::READY)
            {
                text.generate(0, 30, "A: FLAP   B: EXIT", display);
            }
            else if(bird.state == FlappyBird::DEAD)
            {
                char best_line[24] = { 'B', 'E', 'S', 'T', ' ', 0 };
                format_int(bird.best, best_line + 5);
                text.generate(0, -16, "GAME OVER", display);
                text.generate(0, 4, best_line, display);
                text.generate(0, 30, "A: TRY AGAIN", display);
            }
        }

        void update_flappy(bn::sprite_text_generator& text)
        {
            if(bn::keypad::b_pressed())
            {
                go_home(text);
                return;
            }

            bird.update(bn::keypad::a_pressed());

            // pipes: place the 6 pieces of each one around its opening
            for(int i = 0; i < FlappyBird::PIPES; ++i)
            {
                const int x = bird.pipe_x(i);
                const int gap_top = bird.gap_y[i] - FlappyBird::GAP_H / 2;
                const int gap_bottom = bird.gap_y[i] + FlappyBird::GAP_H / 2;
                const bool near_screen = x > -150 && x < 150;

                for(int k = 0; k < 3; ++k)
                {
                    const int top_y = gap_top - 16 - 32 * k;          // pieces going UP from the opening
                    bn::sprite_ptr& up = pipes[i * 6 + k];
                    up.set_position(bn::fixed(x), bn::fixed(top_y));
                    up.set_visible(near_screen && top_y > -96);

                    const int bottom_y = gap_bottom + 16 + 32 * k;    // pieces going DOWN from the opening
                    bn::sprite_ptr& down = pipes[i * 6 + 3 + k];
                    down.set_position(bn::fixed(x), bn::fixed(bottom_y));
                    down.set_visible(near_screen && bottom_y < 96);
                }
            }

            // the bird flaps its wings while alive, and has its own picture when dead
            const int wanted = bird.state == FlappyBird::DEAD ? 2 : int((ticks / 6) % 2);
            if(wanted != bird_frame)
            {
                bird_frame = wanted;
                bird_sprite->set_tiles(bn::sprite_items::flappy_bird.tiles_item(), wanted);
            }
            bird_sprite->set_position(bn::fixed(FlappyBird::BIRD_X), bn::fixed(bird.bird_y()));

            if(bird.score != shown_score || int(bird.state) != shown_state)
            {
                refresh_flappy_text(text);
            }
        }
    };
}

#endif
