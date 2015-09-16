#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include "http.hpp"

namespace http {

// RFC 7230 HTTP/1.1
//
//     fields-value: [,\s]* token (\s* ';' \s* parameter)*
//                   (\s* ',' (\s* token (\s* ';' \s* parameter)*)?)* \s*
//     parameter: token \s* '=' \s* (token | '"' ('\\' qdesc | qdtext)* '"'
//     token: tchar+
//     tchar: [A-Za-z0-9!#$%&'*+\-.^_`|~]
//     qdesc: [\t\x20-\x7e]
//     qdtext: [\t \x21\x23-\x5b\x5d-\x7e]
//
//      S1: tchar   S2  A1{ item.token.push_back (octet); }
//        | ','     S1
//        | [\t ]   S1
//
//      S2: tchar   S2  A1{ item.token.push_back (octet); }
//        | ';'     S4
//        | ','    S12  A5{ fields.push_back (item); }
//        | [\t ]   S3
//        | $      S13  A5{ fields.push_back (item); }
//
//      S3: ';'     S4
//        | ','    S12  A5{ fields.push_back (item); }
//        | [\t ]   S3
//        | $      S13  A5{ fields.push_back (item); }
//
//      S4: tchar   S5  A2{ name.push_back (std::tolower (octet)); }
//        | [\t ]   S4
//
//      S5: tchar   S5  A2{ name.push_back (std::tolower (octet)); }
//        | '='     S7
//        | [\t ]   S6
//
//      S6: '='     S7
//        | [\t ]   S6
//
//      S7: tchar   S8  A3{ value.push_back (octet); }
//        | '"'    S10
//        | [\t ]   S6
//
//      S8: tchar   S8  A3{ value.push_back (octet); }
//        | ';'     S4  A4{ item.parameter.push_back (name, value); }
//        | ','    S12  A6{ A4 (); A5 (); }
//        | [\t ]  S11  A4{ item.parameter.push_back (name, value); }
//        | $      S13  A6{ A4 (); A5 (); }
//
//      S9: qdesc  S10  A3{ value.push_back (octet); }
//
//     S10: qdtext S10  A3{ value.push_back (octet); }
//        | '\\'    S9
//        | '"'    S11
//
//     S11: ';'     S4
//        | ','    S12  A5{ fields.push_back (item); }
//        | [\t ]  S11
//        | $      S13  A5{ fields.push_back (item); }
//
//     S12: tchar   S2  A1{ item.token.push_back (octet); }
//        | ','    S12
//        | [\t ]  S12
//        | $      S13
//
//     S13: MATCH

bool
decode (std::vector<token_type>& fields, std::string const& src, int const lowerlimit)
{
    static const int SHIFT[14][17] = {
    //      0  \d  \h  \T   .  \\   "   =   ;   ,   :  \t  \s  \r  \n   $
        {0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
        {0, 2,  2,  2,  2,  0,  0,  0,  0,  0,  1,  0,  1,  1,  0,  0,  0}, // S1
        {0, 2,  2,  2,  2,  0,  0,  0,  0,  4, 12,  0,  3,  3,  0,  0, 13}, // S2
        {0, 0,  0,  0,  0,  0,  0,  0,  0,  4, 12,  0,  3,  3,  0,  0, 13}, // S3
        {0, 5,  5,  5,  5,  0,  0,  0,  0,  0,  0,  0,  5,  5,  0,  0,  0}, // S4
        {0, 5,  5,  5,  5,  0,  0,  0,  7,  0,  0,  0,  6,  6,  0,  0,  0}, // S5
        {0, 0,  0,  0,  0,  0,  0,  0,  7,  0,  0,  0,  6,  6,  0,  0,  0}, // S6
        {0, 8,  8,  8,  8,  0,  0, 10,  0,  0,  0,  0,  7,  7,  0,  0,  0}, // S7
        {0, 8,  8,  8,  8,  0,  0,  0,  0,  4, 12,  0, 11, 11,  0,  0, 13}, // S8
        {0,10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,  0,  0,  0}, // S9
        {0,10, 10, 10, 10, 10,  9, 11, 10, 10, 10, 10, 10, 10,  0,  0,  0}, // S10
        {0, 0,  0,  0,  0,  0,  0,  0,  0,  4, 12,  0, 11, 11,  0,  0, 13}, // S11
        {0, 2,  2,  2,  2,  0,  0,  0,  0,  0, 12,  0, 12, 12,  0,  0, 13}, // S12
        {1, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // S13
    };
    static const int ACTION[14][17] = {
    //      0  \d  \h  \T   .  \\   "   =   ;   ,   :  \t  \s  \r  \n   $
        {0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
        {0, 1,  1,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // S1
        {0, 1,  1,  1,  1,  0,  0,  0,  0,  0,  5,  0,  0,  0,  0,  0,  5}, // S2
        {0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  5,  0,  0,  0,  0,  0,  5}, // S3
        {0, 2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // S4
        {0, 2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // S5
        {0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // S6
        {0, 3,  3,  3,  3,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // S7
        {0, 3,  3,  3,  3,  0,  0,  0,  0,  4,  6,  0,  4,  4,  0,  0,  6}, // S8
        {0, 3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  0,  0,  0}, // S9
        {0, 3,  3,  3,  3,  3,  0,  4,  3,  3,  3,  3,  3,  3,  0,  0,  0}, // S10
        {0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  5,  0,  0,  0,  0,  0,  5}, // S11
        {0, 1,  1,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // S12
        {0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // S13
    };
    std::vector<token_type> list;
    token_type item;
    std::string::const_iterator s = src.cbegin ();
    std::string::const_iterator const e = src.cend ();
    std::string name;
    std::string value;
    int next_state = 1 == lowerlimit ? 1 : 12;
    for (; s <= e; ++s) {
        int octet = s == e ? '\0' : static_cast<uint8_t> (*s);
        int cls = s == e ? 16 : tocclass (octet);
        int prev_state = next_state;
        next_state = SHIFT[prev_state][cls];
        if (! next_state)
            break;
        switch (ACTION[prev_state][cls]) {
        case 1:
            item.token.push_back (octet);
            break;
        case 2:
            name.push_back (std::tolower (octet));
            break;
        case 3:
            value.push_back (octet);
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
    if (! (SHIFT[next_state][0] & 1))
        return false;
    std::swap (fields, list);
    return true;
}

}//namespace http
