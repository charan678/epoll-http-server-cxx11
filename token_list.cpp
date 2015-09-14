#include "server.hpp"

std::size_t
token_list_type::find (std::string const& x) const
{
    for (std::size_t i = 0; i < list.size (); ++i)
        if (list[i].equal_token (x))
            return i;
    return list.size ();
}

// token only without parameters.
bool
token_list_type::decode_simple (std::string const& str, int const min)
{
    static const int8_t SHIFT[6][5] = {
    //         \s   ,   T   $
        {   0,  0,  0,  0,  0},
        {   0,  1,  1,  2,  0}, // S1: \s S1 | ',' S1 | T S2
        {   0,  3,  4,  2,  5}, // S2: \s S3 | ',' S4 | T S2 | $ S5
        {   0,  3,  4,  0,  5}, // S3: \s S3 | ',' S4 | $ S5
        {   0,  4,  4,  2,  5}, // S4: \s S4 | ',' S4 | T S2 | $ S5
        {   1,  0,  0,  0,  0}, // S5: MATCH
    };
    static const int8_t CCLS[128] = {
    //                                     \t  \n          \r
        0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    //      !   "   #   $   %   &   '   (   )   *   +   ,   -   .   /
        1,  3,  0,  3,  3,  3,  3,  3,  0,  0,  3,  3,  2,  3,  3,  0,
    //  0   1   2   3   4   5   6   7   8   9   :   ;   <   =   >   ?
        3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  0,  0,  0,  0,  0,  0,
    //  @   A   B   C   D   E   F   G   H   I   J   K   L   M   N   O
        0,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    //  P   Q   R   S   T   U   V   W   X   Y   Z   [  \\   ]   ^   _
        3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  0,  0,  0,  3,  3,
    //  `   a   b   c   d   e   f   g   h   i   j   k   l   m   n   o
        3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    //  p   q   r   s   t   u   v   w   x   y   z   {   |   }   ~
        3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  0,  3,  0,  3,  0,
    };
    token_type item;
    std::string::const_iterator s = str.cbegin ();
    std::string::const_iterator const e = str.cend ();
    int next_state = 1 == min ? 1 : 4;
    for (; s <= e; ++s) {
        int ch = s == e ? '\0' : *s;
        int cls = s == e ? 4 : ch < 128 ? CCLS[ch] : 0;
        int prev_state = next_state;
        next_state = SHIFT[prev_state][cls];
        if (! next_state)
            break;
        if (3 == cls)
            item.token.push_back (ch);
        else if (2 == prev_state) {
            list.push_back (item);
            item.clear ();
        }
    }
    return (SHIFT[next_state][0] & 1) != 0;
}

// token with parameters.
bool
token_list_type::decode (std::string const& str, int const min)
{
    static const int8_t SHIFT[14][9] = {
    //         \s   ,   ;   =   "  \\   T   $
        {   0,  0,  0,  0,  0,  0,  0,  0,  0},
        {   0,  1,  1,  0,  0,  0,  0,  2,  0}, // S1: \s S1 | ',' S1 | T S2
        {   0,  3, 12,  4,  0,  0,  0,  2, 13}, // S2: \s S3 | ',' S12 | ';' S4 | T S2 | $ S13
        {   0,  3, 12,  4,  0,  0,  0,  0, 13}, // S3: \s S3 | ',' S12 | ';' S4 | $ S13
        {   0,  4,  0,  0,  0,  0,  0,  5,  0}, // S4: \s S4 | T S5
        {   0,  6,  0,  0,  7,  0,  0,  5,  0}, // S5: \s S6 | '=' S7 | T S5
        {   0,  6,  0,  0,  7,  0,  0,  0,  0}, // S6: \s S6 | '=' S7
        {   0,  7,  0,  0,  0, 10,  0,  8,  0}, // S7: \s S7 | '"' S10 | T S8
        {   0, 11, 12,  4,  0,  0,  0,  8, 13}, // S8: \s S11 | ',' S12 | ';' S4 | T S8 | $ S13
        {   0, 10, 10, 10, 10, 10, 10, 10,  0}, // S9: . S10
        {   0, 10, 10, 10, 10, 11,  9, 10,  0}, // S10: '"' S11 / '\\' S9 / . S10
        {   0, 11, 12,  4,  0,  0,  0,  0, 13}, // S11: \s S11 | ',' S12 | ';' S4 | $ S13
        {   0, 12, 12,  0,  0,  0,  0,  2, 13}, // S12: \s S12 | ',' S12 | T S2 | $ S13
        {   1,  0,  0,  0,  0,  0,  0,  0,  0}, // S13: MATCH
    };
    static const int8_t ACTION[14][9] = {
    //         \s   ,   ;   =   "  \\   T   $
        {   0,  0,  0,  0,  0,  0,  0,  0,  0},
        {   0,  0,  0,  0,  0,  0,  0,  1,  0}, // S1: \s S1 | ',' S1 | T S2
        {   0,  0,  5,  0,  0,  0,  0,  1,  5}, // S2: \s S3 | ',' S12 | ';' S4 | T S2 | $ S13
        {   0,  0,  5,  0,  0,  0,  0,  0,  5}, // S3: \s S3 | ',' S12 | ';' S4 | $ S13
        {   0,  0,  0,  0,  0,  0,  0,  2,  0}, // S4: \s S4 | T S5
        {   0,  0,  0,  0,  0,  0,  0,  2,  0}, // S5: \s S6 | '=' S7 | T S5
        {   0,  0,  0,  0,  0,  0,  0,  0,  0}, // S6: \s S6 | '=' S7
        {   0,  0,  0,  0,  0,  0,  0,  3,  0}, // S7: \s S7 | '"' S10 | T S8
        {   0,  4,  6,  4,  0,  0,  0,  3,  6}, // S8: \s S11 | ',' S12 | ';' S4 | T S8 | $ S13
        {   0,  3,  3,  3,  3,  3,  3,  3,  0}, // S9: . S10
        {   0,  3,  3,  3,  3,  4,  0,  3,  0}, // S10: '"' S11 / '\\' S9 / . S10
        {   0,  0,  5,  0,  0,  0,  0,  0,  5}, // S11: \s S11 | ',' S12 | ';' S4 | $ S13
        {   0,  0,  0,  0,  0,  0,  0,  1,  0}, // S12: \s S12 | ',' S12 | T S2 | $ S13
        {   0,  0,  0,  0,  0,  0,  0,  0,  0}, // S13: MATCH
    };
    static const int8_t CCLS[128] = {
    //                                     \t  \n          \r
        0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    //      !   "   #   $   %   &   '   (   )   *   +   ,   -   .   /
        1,  7,  5,  7,  7,  7,  7,  7,  0,  0,  7,  7,  2,  7,  7,  0,
    //  0   1   2   3   4   5   6   7   8   9   :   ;   <   =   >   ?
        7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  0,  3,  0,  4,  0,  0,
    //  @   A   B   C   D   E   F   G   H   I   J   K   L   M   N   O
        0,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
    //  P   Q   R   S   T   U   V   W   X   Y   Z   [  \\   ]   ^   _
        7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  0,  6,  0,  7,  7,
    //  `   a   b   c   d   e   f   g   h   i   j   k   l   m   n   o
        7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
    //  p   q   r   s   t   u   v   w   x   y   z   {   |   }   ~
        7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  0,  7,  0,  7,  0,
    };
    token_type item;
    std::string::const_iterator s = str.cbegin ();
    std::string::const_iterator const e = str.cend ();
    std::string name;
    std::string value;
    int next_state = 1 == min ? 1 : 12;
    for (; s <= e; ++s) {
        int ch = s == e ? '\0' : *s;
        int cls = s == e ? 8 : ch < 128 ? CCLS[ch] : 0;
        int prev_state = next_state;
        if ((9 == prev_state || 10 == prev_state) && 0 == cls)
            cls = 0x20 <= ch && ch < 0x7f ? 7 : 0;
        next_state = SHIFT[prev_state][cls];
        if (! next_state)
            break;
        switch (ACTION[prev_state][cls]) {
        case 1:
            item.token.push_back (ch);
            break;
        case 2:
            name.push_back (std::tolower (ch));
            break;
        case 3:
            value.push_back (ch);
            break;
        case 4:
            item.parameter.push_back (name);
            item.parameter.push_back (value);
            name.clear ();
            value.clear ();
            break;
        case 5:
            list.push_back (item);
            item.clear ();
            break;
        case 6:
            item.parameter.push_back (name);
            item.parameter.push_back (value);
            list.push_back (item);
            name.clear ();
            value.clear ();
            item.clear ();
            break;
        }
    }
    return (SHIFT[next_state][0] & 1) != 0;
}

