#include <string>
#include <vector>
#include <cctype>
#include <algorithm>
#include "http.hpp"

namespace http {

// RFC 7230 HTTP/1.1
//
//     fields-value: [,\s]* token (\s* ',' (\s* token)?)* \s*
//     token: tchar+
//     tchar: [A-Za-z0-9!#$%&'*+\-.^_`|~]
//
//      S1: tchar  S2 A1{ item.token.push_back (std::tolower (ch)); }
//        | ','    S1
//        | [\t ]  S1
//
//      S2: tchar  S2 A1{ item.token.push_back (std::tolower (ch)); }
//        | ','    S4 A2{ fields.push_back (item); }
//        | [\t ]  S3 A2{ fields.push_back (item); }
//        | $      S5 A2{ fields.push_back (item); }
//
//      S3: ','    S4
//        | [\t ]  S3
//        | $      S5
//
//      S4: tchar  S2 A1{ item.token.push_back (std::tolower (ch)); }
//        | ','    S4
//        | [\t ]  S4
//        | $      S5
//
//      S5: MATCH

bool
decode (std::vector<simple_token_type>& fields, std::string const& src, int const lowerlimit)
{
    static const int SHIFT[6][17] = {
    //      0  \d  \h  \T   .  \\   "   =   ;   ,   :  \t  \s  \r  \n   $
        {0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
        {0, 2,  2,  2,  2,  0,  0,  0,  0,  0,  1,  0,  1,  1,  0,  0,  0}, // S1:
        {0, 2,  2,  2,  2,  0,  0,  0,  0,  0,  4,  0,  3,  3,  0,  0,  5}, // S2:
        {0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  4,  0,  3,  3,  0,  0,  5}, // S3:
        {0, 2,  2,  2,  2,  0,  0,  0,  0,  0,  4,  0,  4,  4,  0,  0,  5}, // S4:
        {1, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // S5:
    };
    std::vector<simple_token_type> list;
    std::string::const_iterator s = src.cbegin ();
    std::string::const_iterator const e = src.cend ();
    simple_token_type item;
    int next_state = 1 == lowerlimit ? 1 : 4;
    for (; s <= e; ++s) {
        int ch = s == e ? '\0' : static_cast<uint8_t> (*s);
        int cls = s == e ? 16 : tocclass (ch);
        int prev_state = next_state;
        next_state = cls ? SHIFT[prev_state][cls] : 0;
        if (! next_state)
            break;
        if (cls <= 4)
            item.token.push_back (std::tolower (ch));
        else if (2 == prev_state) {
            list.push_back (item);
            item.clear ();
        }
    }
    if (! (SHIFT[next_state][0] & 1))
        return false;
    std::swap (fields, list);
    return true;
}

}//namespace http
