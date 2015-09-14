#include <string>
#include <cctype>
#include "server.hpp"

ssize_t
message_type::canonical_length ()
{
    static const int8_t SHIFT[6][5] = {
    //         \s   ,  \d   $
        {   0,  0,  0,  0,  0},
        {   0,  1,  1,  2,  0}, // S1: \s S1 | ',' S1 | \d S2
        {   0,  3,  4,  2,  5}, // S2: \s S3 | ',' S4 | \d S2 | $ S5
        {   0,  3,  4,  0,  5}, // S3: \s S3 | ',' S4 |       | $ S5
        {   0,  4,  4,  2,  5}, // S4: \s S4 | ',' S4 | \d S2 | $ S5
        {   1,  0,  0,  0,  0}, // S5: MATCH
    };
    std::string field = header["content-length"];
    std::string::const_iterator s = field.cbegin ();
    std::string::const_iterator const e = field.cend ();
    ssize_t length = -1;
    ssize_t value = 0;
    int next_state = 1;
    for (; s <= e; ++s) {
        int c = s == e ? '\0' : *s;
        int cls = s == e ? 4 : '\t' == c ? 1 : ' ' == c ? 1
                : ',' == c ? 2
                : std::isdigit (c) ? 3 : 0;
        int prev_state = next_state;
        next_state = cls == 0 ? 0 : SHIFT[prev_state][cls];
        if (! next_state)
            return -400;
        if (3 == cls) {
            // 214748364L * 10 + 7 == 0x7fffffffL
            if (value > 214748364L || (value == 214748364L && c > '7'))
                return -413;
            value = value * 10 + c - '0';
        }
        else if (2 == prev_state) {
            if (length >= 0 && length != value)
                return -400;
            length = value;
            value = 0;
        }
    }
    return (1 & SHIFT[next_state][0]) ? length : -400;
}
