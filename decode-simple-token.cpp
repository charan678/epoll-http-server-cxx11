#include <string>
#include <vector>
#include <cctype>
#include <algorithm>
#include "http.hpp"

namespace http {

// RFC 7230 HTTP/1.1
//
//      fields-value: [,\s]* token (\s* ',' (\s* token)?)* \s*
//      token: tchar+
//      tchar: [A-Za-z0-9!#$%&'*+\-.^_`|~]
//
//      S1: tchar  S2 A1{ item.token.push_back (std::tolower (ch)); }
//        | [,]    S1
//        | [\t ]  S1
//
//      S2: tchar  S2 A1{ item.token.push_back (std::tolower (ch)); }
//        | [,]    S4 A2{ fields.push_back (item); }
//        | [\t ]  S3 A2{ fields.push_back (item); }
//        | $      S5 A2{ fields.push_back (item); }
//
//      S3: [,]    S4
//        | [\t ]  S3
//        | $      S5
//
//      S4: tchar  S2 A1{ item.token.push_back (std::tolower (ch)); }
//        | [,]    S4
//        | [\t ]  S4
//        | $      S5
//
//      S5: MATCH

static inline int
lookup_cls (uint32_t const tbl[], uint32_t const octet)
{
    uint32_t const i = octet >> 3;
    uint32_t const count = (7 - (octet & 7)) << 2;
    return octet < 128 ? ((tbl[i] >> count) & 0x0f) : 0;
}

bool
decode (std::vector<simple_token_type>& fields, std::string const& src, int const lowerlimit)
{
    static const int8_t SHIFT[6][5] = {
    //      tchar [,]   [\t ] $
        {0, 0x00, 0x00, 0x00, 0x00},
        {0, 0x12, 0x01, 0x01, 0x00}, // S1
        {0, 0x12, 0x24, 0x23, 0x25}, // S2
        {0, 0x00, 0x04, 0x03, 0x05}, // S3
        {0, 0x12, 0x04, 0x04, 0x05}, // S4
        {1, 0x00, 0x00, 0x00, 0x00}, // S5
    };
    static const uint32_t CCLASS[16] = {
    //                 tn  r
        0x00000000, 0x03000000, 0x00000000, 0x00000000,
    //     !"#$%&'    ()*+,-./    01234567    89:;<=>?
        0x31011111, 0x00112110, 0x11111111, 0x11000000,
    //    @ABCDEFG    HIJKLMNO    PQRSTUVW    XYZ[\]^_
        0x01111111, 0x11111111, 0x11111111, 0x11100011,
    //    `abcdefg    hijklmno    pqrstuvw    xyz{|}~ 
        0x11111111, 0x11111111, 0x11111111, 0x11101010,
    };
    std::vector<simple_token_type> list;
    std::string::const_iterator s = src.cbegin ();
    std::string::const_iterator const e = src.cend ();
    simple_token_type item;
    bool matched = false;
    int next_state = 1 == lowerlimit ? 1 : 4;
    for (; s <= e; ++s) {
        uint32_t octet = s == e ? '\0' : static_cast<uint8_t> (*s);
        int cls = s == e ? 4 : lookup_cls (CCLASS, octet);
        int prev_state = next_state;
        next_state = ! cls ? 0 : SHIFT[prev_state][cls] & 0x0f;
        if (! next_state)
            break;
        switch (SHIFT[prev_state][cls] & 0xf0) {
        case 0x10:
            item.token.push_back (std::tolower (octet));
            break;
        case 0x20:
            list.push_back (item);
            item.clear ();
            break;
        }
        if (SHIFT[next_state][0] & 1)
            matched = true;
    }
    if (matched)
        std::swap (fields, list);
    return matched;
}

}//namespace http
