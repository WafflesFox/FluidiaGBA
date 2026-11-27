#ifndef PHONE_LOGIC_H
#define PHONE_LOGIC_H

// ================================================================================================
// The RULES of the phone apps. No Butano in this file, only plain C++, so nothing here draws anything.
// (phone.h is the part that draws; it asks this file "what is the display showing?" / "where is the bird?")
// ================================================================================================

namespace phone
{
    // writes a whole number into out ("-42"); out needs room for 12 characters
    inline void format_int(int value, char* out)
    {
        int n = 0;
        unsigned v = value < 0 ? 0u - unsigned(value) : unsigned(value);
        if(value < 0) { out[n++] = '-'; }

        char tmp[12];
        int t = 0;
        if(v == 0) { tmp[t++] = '0'; }
        while(v > 0) { tmp[t++] = char('0' + v % 10); v /= 10; }
        while(t > 0) { out[n++] = tmp[--t]; }
        out[n] = 0;
    }

    // ============================================================================================
    // TEXT WRAPPING (used by the changelog)
    // ============================================================================================

    struct TextLine { char text[16]; };      // one line of at most 15 characters

    // Splits s into lines of at most `width` characters (width must be 15 or less), breaking at spaces.
    // Writes them to out and returns how many lines it made (never more than max_lines).
    inline int wrap_text(const char* s, int width, TextLine* out, int max_lines)
    {
        int len = 0;
        while(s[len] != 0) { ++len; }

        int count = 0;
        int pos = 0;
        while(pos < len && count < max_lines)
        {
            while(pos < len && s[pos] == ' ') { ++pos; }                  // skip the spaces between words
            if(pos >= len) { break; }

            int end = pos + width;
            if(end >= len)
            {
                end = len;                                                 // the rest fits on this line
            }
            else
            {
                int cut = end;                                             // find the last space inside the line
                while(cut > pos && s[cut] != ' ') { --cut; }
                if(cut > pos) { end = cut; }                               // (no space at all: cut the long word)
            }

            while(end > pos && s[end - 1] == ' ') { --end; }              // no trailing spaces

            int n = 0;
            for(int i = pos; i < end; ++i) { out[count].text[n++] = s[i]; }
            out[count].text[n] = 0;
            ++count;
            pos = end;
        }
        return count;
    }
    struct Options
    {
        bool battle_3d = true;
        bool battle_tint = true;
    };

    struct Stopwatch
    {
        bool running = false;
        int frames = 0;                                     // the game runs at 60 frames per second

        void update() { if(running && frames < 99 * 60 * 60) { ++frames; } }   // stops at 99 minutes
        void toggle() { running = !running; }
        void reset()  { running = false; frames = 0; }

        void format(char* out) const                        // writes "MM:SS.t"; out needs 8 characters
        {
            const int seconds_total = frames / 60;
            const int tenths = (frames % 60) / 6;
            const int seconds = seconds_total % 60;
            const int minutes = (seconds_total / 60) % 100;
            out[0] = char('0' + minutes / 10);  out[1] = char('0' + minutes % 10);  out[2] = ':';
            out[3] = char('0' + seconds / 10);  out[4] = char('0' + seconds % 10);  out[5] = '.';
            out[6] = char('0' + tenths);        out[7] = 0;
        }
    };


    // ============================================================================================
    // CALCULATOR
    // A pocket calculator: 2 + 3 * 4 = gives 20 (it calculates as you go, like most real ones).
    // Numbers are stored in thousandths (so 1.5 is 1500), which means no floating point is needed.
    // ============================================================================================

    struct Calculator
    {
        static constexpr long long LIMIT = 999999999;   // biggest number: 999999.999
        static constexpr int MAX_INT_DIGITS = 6;
        static constexpr int MAX_DECIMALS = 3;

        long long acc = 0;          // the running total, in thousandths
        char op = 0;                // the operator waiting for its second number: + - * /  (0 = none)
        char entry[12] = {};        // the digits being typed, like "12.5"
        int entry_len = 0;
        bool entry_neg = false;     // the number being typed is negative
        bool error = false;         // divided by zero, or the number got too big
        bool just_equaled = false;  // the last key was "=" (typing a digit now starts a fresh sum)

        void clear() { *this = Calculator(); }

        // keys: '0'..'9'  '.'  '+' '-' '*' '/'  '='  'C' clear  '<' backspace  'N' negate  'Z' the "00" key
        void press(char key)
        {
            if(error)
            {
                clear();
                if(key == 'C') { return; }
            }

            switch(key)
            {
                case 'C': clear(); break;
                case '<': backspace(); break;
                case 'N': negate(); break;
                case '.': type_dot(); break;
                case 'Z': type_digit('0'); type_digit('0'); break;
                case '+': case '-': case '*': case '/': press_operator(key); break;
                case '=': press_equals(); break;
                default:
                    if(key >= '0' && key <= '9') { type_digit(key); }
                    break;
            }
        }

        // what the big display shows: the number being typed, or else the total
        void main_line(char* out) const
        {
            if(error) { copy("ERR", out); return; }

            if(entry_len > 0)
            {
                int n = 0;
                if(entry_neg) { out[n++] = '-'; }
                for(int i = 0; i < entry_len; ++i) { out[n++] = entry[i]; }
                out[n] = 0;
            }
            else
            {
                format_milli(acc, out);
            }
        }

        // the small line above it: the total and the operator that is waiting, like "12 +"
        void op_line(char* out) const
        {
            if(op == 0 || error) { out[0] = 0; return; }

            format_milli(acc, out);
            int n = 0;
            while(out[n] != 0) { ++n; }
            out[n++] = ' ';
            out[n++] = op;
            out[n] = 0;
        }

        // turns thousandths into text: 1500 -> "1.5", -2000 -> "-2"
        static void format_milli(long long v, char* out)
        {
            int n = 0;
            if(v < 0) { out[n++] = '-'; v = -v; }

            const long long whole = v / 1000;
            const int frac = int(v % 1000);

            char tmp[16];
            int t = 0;
            long long x = whole;
            if(x == 0) { tmp[t++] = '0'; }
            while(x > 0) { tmp[t++] = char('0' + x % 10); x /= 10; }
            while(t > 0) { out[n++] = tmp[--t]; }

            if(frac != 0)
            {
                char digits[3] = { char('0' + frac / 100), char('0' + (frac / 10) % 10), char('0' + frac % 10) };
                int len = 3;
                while(len > 0 && digits[len - 1] == '0') { --len; }   // 1.500 -> 1.5
                out[n++] = '.';
                for(int i = 0; i < len; ++i) { out[n++] = digits[i]; }
            }
            out[n] = 0;
        }

    private:
        static void copy(const char* from, char* out)
        {
            int i = 0;
            while(from[i] != 0) { out[i] = from[i]; ++i; }
            out[i] = 0;
        }

        int dot_index() const
        {
            for(int i = 0; i < entry_len; ++i)
            {
                if(entry[i] == '.') { return i; }
            }
            return -1;
        }

        void clear_entry()
        {
            entry_len = 0;
            entry[0] = 0;
            entry_neg = false;
        }

        void start_fresh_if_needed()
        {
            if(just_equaled)            // typing right after "=" starts a brand new sum
            {
                acc = 0;
                op = 0;
                just_equaled = false;
            }
        }

        void type_digit(char c)
        {
            start_fresh_if_needed();
            const int dot = dot_index();

            if(dot < 0 && entry_len == 1 && entry[0] == '0')   // "0" followed by a digit: replace the zero
            {
                if(c == '0') { return; }
                entry_len = 0;
            }

            if(dot >= 0) { if(entry_len - dot - 1 >= MAX_DECIMALS) { return; } }
            else         { if(entry_len >= MAX_INT_DIGITS) { return; } }

            entry[entry_len++] = c;
            entry[entry_len] = 0;
        }

        void type_dot()
        {
            start_fresh_if_needed();
            if(dot_index() >= 0) { return; }

            if(entry_len == 0) { entry[entry_len++] = '0'; }
            entry[entry_len++] = '.';
            entry[entry_len] = 0;
        }

        void backspace()
        {
            if(entry_len > 0)
            {
                entry[--entry_len] = 0;
                if(entry_len == 0) { entry_neg = false; }
            }
        }

        void negate()
        {
            if(entry_len > 0)        { entry_neg = !entry_neg; }
            else if(just_equaled)    { acc = -acc; }
        }

        long long entry_value() const
        {
            long long whole = 0;
            long long frac = 0;
            int frac_digits = 0;
            bool after_dot = false;

            for(int i = 0; i < entry_len; ++i)
            {
                if(entry[i] == '.')      { after_dot = true; }
                else if(!after_dot)      { whole = whole * 10 + (entry[i] - '0'); }
                else                     { frac = frac * 10 + (entry[i] - '0'); ++frac_digits; }
            }
            while(frac_digits < MAX_DECIMALS) { frac *= 10; ++frac_digits; }

            const long long v = whole * 1000 + frac;
            return entry_neg ? -v : v;
        }

        long long apply(long long a, char o, long long b)
        {
            long long r = 0;
            switch(o)
            {
                case '+': r = a + b; break;
                case '-': r = a - b; break;
                case '*': r = a * b / 1000; break;
                case '/':
                    if(b == 0) { error = true; return 0; }
                    r = a * 1000 / b;
                    break;
                default:  r = b; break;
            }

            if(r > LIMIT || r < -LIMIT) { error = true; return 0; }
            return r;
        }

        void press_operator(char k)
        {
            if(entry_len > 0)
            {
                const long long v = entry_value();
                acc = (op != 0) ? apply(acc, op, v) : v;
                if(error) { return; }
                clear_entry();
            }
            op = k;                  // (with nothing typed yet this just swaps the waiting operator)
            just_equaled = false;
        }

        void press_equals()
        {
            if(entry_len == 0) { return; }

            const long long v = entry_value();
            acc = (op != 0) ? apply(acc, op, v) : v;
            if(error) { return; }

            op = 0;
            clear_entry();
            just_equaled = true;
        }
    };

    // ============================================================================================
    // FLAPINGBIRD
    // Positions are plain pixels with (0, 0) in the middle of the screen, y going DOWN.
    // Movement uses 1/16 pixels so gravity can be gentle without floating point.
    // ============================================================================================

    struct FlappyBird
    {
        enum State { READY, PLAYING, DEAD };

        static constexpr int PIPES = 3;
        static constexpr int SCALE = 16;            // sub-pixel precision
        static constexpr int HALF_SCREEN_W = 120;
        static constexpr int HALF_SCREEN_H = 80;

        static constexpr int BIRD_X = -50;          // the bird never moves sideways, the pipes come to it
        static constexpr int BIRD_W = 14;           // how big it is drawn...
        static constexpr int BIRD_H = 12;
        static constexpr int HIT_W = 10;            // ...and how big it is for collisions (a little forgiving)
        static constexpr int HIT_H = 8;

        static constexpr int PIPE_W = 24;
        static constexpr int GAP_H = 56;            // the opening you fly through
        static constexpr int PIPE_SPACING = 100;    // distance between pipes
        static constexpr int PIPE_EDGE = 90;        // pipes are drawn from this far above/below the screen centre

        static constexpr int SPEED = 24;            // pipe speed: 1.5 px per frame
        static constexpr int GRAVITY = 4;           // 0.25 px per frame, every frame
        static constexpr int FLAP = -56;            // a flap launches the bird upward at 3.5 px per frame
        static constexpr int MAX_FALL = 64;

        State state = READY;
        int bird_y16 = 0;
        int vy = 0;
        int pipe_x16[PIPES] = {};
        int gap_y[PIPES] = {};                      // the middle of each opening
        bool passed[PIPES] = {};
        int score = 0;
        int best = 0;
        int dead_timer = 0;
        int bob_timer = 0;
        unsigned rng = 1;

        void reset(unsigned seed)
        {
            rng = seed * 2654435761u + 12345u;
            if(rng == 0) { rng = 1; }

            state = READY;
            bird_y16 = 0;
            vy = 0;
            score = 0;
            dead_timer = 0;
            bob_timer = 0;

            for(int i = 0; i < PIPES; ++i)
            {
                pipe_x16[i] = (HALF_SCREEN_W + 40 + i * PIPE_SPACING) * SCALE;
                gap_y[i] = random_gap();
                passed[i] = false;
            }
        }

        // a_pressed: was A pressed this frame
        void update(bool a_pressed)
        {
            switch(state)
            {
                case READY:
                {
                    ++bob_timer;                                      // the bird floats up and down while waiting
                    const int phase = bob_timer % 60;
                    const int wave = phase < 30 ? phase - 15 : 45 - phase;   // -15 .. 15
                    bird_y16 = (wave / 3) * SCALE;

                    if(a_pressed)
                    {
                        state = PLAYING;
                        vy = FLAP;
                    }
                    break;
                }

                case PLAYING:
                    if(a_pressed) { vy = FLAP; }
                    fall();

                    if(bird_y16 < (-HALF_SCREEN_H + BIRD_H / 2) * SCALE)     // the ceiling just stops you
                    {
                        bird_y16 = (-HALF_SCREEN_H + BIRD_H / 2) * SCALE;
                        if(vy < 0) { vy = 0; }
                    }

                    move_pipes();

                    if(touches_something()) { state = DEAD; vy = 0; dead_timer = 0; }
                    break;

                case DEAD:
                    ++dead_timer;
                    fall();
                    if(a_pressed && dead_timer > 30 && on_floor()) { reset(rng); }   // press A to try again
                    break;
                default:
                    break;
            }
        }

        int bird_y() const { return bird_y16 / SCALE; }
        int pipe_x(int i) const { return pipe_x16[i] / SCALE; }

    private:
        unsigned next_random()
        {
            rng ^= rng << 13;
            rng ^= rng >> 17;
            rng ^= rng << 5;
            return rng;
        }

        // somewhere that keeps the whole opening on screen, with a little margin
        int random_gap()
        {
            const int limit = HALF_SCREEN_H - 12 - GAP_H / 2;         // 40
            return -limit + int(next_random() % unsigned(2 * limit + 1));
        }

        bool on_floor() const { return bird_y16 >= (HALF_SCREEN_H - BIRD_H / 2) * SCALE; }

        void fall()
        {
            vy += GRAVITY;
            if(vy > MAX_FALL) { vy = MAX_FALL; }
            bird_y16 += vy;

            if(state == DEAD && on_floor())
            {
                bird_y16 = (HALF_SCREEN_H - BIRD_H / 2) * SCALE;
                vy = 0;
            }
        }

        void move_pipes()
        {
            for(int i = 0; i < PIPES; ++i)
            {
                pipe_x16[i] -= SPEED;

                if(pipe_x16[i] < (-HALF_SCREEN_W - PIPE_W) * SCALE)       // left the screen: reuse it on the far right
                {
                    pipe_x16[i] += PIPES * PIPE_SPACING * SCALE;
                    gap_y[i] = random_gap();
                    passed[i] = false;
                }

                if(!passed[i] && pipe_x16[i] + (PIPE_W / 2) * SCALE < BIRD_X * SCALE)   // the bird is past it
                {
                    passed[i] = true;
                    ++score;
                    if(score > best) { best = score; }
                }
            }
        }

        bool touches_something() const
        {
            const int half_w = (PIPE_W / 2 + HIT_W / 2) * SCALE;
            const int bird_top = bird_y16 - (HIT_H / 2) * SCALE;
            const int bird_bottom = bird_y16 + (HIT_H / 2) * SCALE;

            if(bird_bottom >= HALF_SCREEN_H * SCALE) { return true; }    // hit the ground

            for(int i = 0; i < PIPES; ++i)
            {
                int dx = pipe_x16[i] - BIRD_X * SCALE;
                if(dx < 0) { dx = -dx; }

                if(dx < half_w)                                          // lined up with this pipe
                {
                    const int gap_top = (gap_y[i] - GAP_H / 2) * SCALE;
                    const int gap_bottom = (gap_y[i] + GAP_H / 2) * SCALE;
                    if(bird_top < gap_top || bird_bottom > gap_bottom) { return true; }
                }
            }
            return false;
        }
    };
}

#endif
