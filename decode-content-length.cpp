#include <string>
#include <cctype>
#include "http.hpp"

namespace http {

bool
decode (content_length_type& field, std::string const& src)
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
    std::string::const_iterator s = src.cbegin ();
    std::string::const_iterator const e = src.cend ();
    ssize_t length = -1;
    ssize_t value = 0;
    int next_state = 1;
    field.status = 400;
    field.length = 0;
    for (; s <= e; ++s) {
        int c = s == e ? '\0' : *s;
        int cls = s == e ? 4 : '\t' == c ? 1 : ' ' == c ? 1
                : ',' == c ? 2
                : std::isdigit (c) ? 3 : 0;
        int prev_state = next_state;
        next_state = cls == 0 ? 0 : SHIFT[prev_state][cls];
        if (! next_state)
            return false;
        if (3 == cls) {
            // 214748364L * 10 + 7 == 0x7fffffffL
            if (value > 214748364L || (value == 214748364L && c > '7')) {
                field.status = 413;
                return false;
            }
            value = value * 10 + c - '0';
        }
        else if (2 == prev_state) {
            if (length >= 0 && length != value)
                return false;
            length = value;
            value = 0;
        }
    }
    if (! (1 & SHIFT[next_state][0]))
        return false;
    field.status = 200;
    field.length = length;
    return true;
}

}//namespace http
