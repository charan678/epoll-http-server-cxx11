#include <string>
#include "http.hpp"

namespace http {

// S1: 'W' S3 | '"' S5 | '*' S8 | ',' S2 | [\t ] S1
// S2: 'W' S3 | '"' S5 | ',' S2 | [\t ] S2
// S3: '/' S4
// S4: '"' S5
// S5: [\x21\x23-\x7e] S5 | '"' S6
// S6: ',' S7 | [\t ] S6 | $ S9
// S7: 'W' S3 | '"' S5 | ',' S7 | [\t ] S7 | $ S9
// S8: [\t ] S8 | $ S9
// S9: MATCH

static inline int
lookup_cls (uint32_t const tbl[], uint32_t const octet)
{
    uint32_t const i = octet >> 3;
    uint32_t const count = (7 - (octet & 7)) << 2;
    return octet < 128 ? ((tbl[i] >> count) & 0x0f) : 0;
}

bool
decode (std::vector<etag_type>& fields, std::string const& src)
{
    static const int SHIFT[10][9] = {
    //      qch   [W]   [/]   ["]   [*]   [,]   [\t ] $
        {0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        {0, 0x00, 0x13, 0x00, 0x25, 0x38, 0x02, 0x01, 0x00}, // S1
        {0, 0x00, 0x13, 0x00, 0x25, 0x00, 0x02, 0x02, 0x00}, // S2
        {0, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00}, // S3
        {0, 0x00, 0x00, 0x00, 0x25, 0x00, 0x00, 0x00, 0x00}, // S4
        {0, 0x25, 0x25, 0x25, 0x36, 0x25, 0x25, 0x00, 0x00}, // S5
        {0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x06, 0x09}, // S6
        {0, 0x00, 0x13, 0x00, 0x25, 0x00, 0x07, 0x07, 0x09}, // S7
        {0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x09}, // S8
        {1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // S9
    };
    static const uint32_t CCLASS[16] = {
    //                 tn  r                          
        0x00000000, 0x07000000, 0x00000000, 0x00000000,
    //     !"#$%&'    ()*+,-./    01234567    89:;<=>?
        0x71411111, 0x11516113, 0x11111111, 0x11111111,
    //    @ABCDEFG    HIJKLMNO    PQRSTUVW    XYZ[\]^_
        0x11111111, 0x11111111, 0x11111112, 0x11111111,
    //    `abcdefg    hijklmno    pqrstuvw    xyz{|}~ 
        0x11111111, 0x11111111, 0x11111111, 0x11111110,
    };
    std::string::const_iterator s = src.cbegin ();
    std::string::const_iterator const e = src.cend ();
    std::vector<etag_type> list;
    etag_type etag;
    bool matched = false;    
    for (int next_state = 1; s <= e; ++s) {
        uint32_t octet = s == e ? '\0' : static_cast<uint8_t> (*s);
        int cls = s == e ? 8 : lookup_cls (CCLASS, octet);
        int prev_state = next_state;
        next_state = ! cls ? 0 : (SHIFT[prev_state][cls] & 0x0f);
        if (! next_state)
            break;
        switch (SHIFT[prev_state][cls] & 0xf0) {
        case 0x10:
            etag.weak = true;
            break;
        case 0x20:
            etag.opaque.push_back (octet);
            break;
        case 0x30:
            etag.opaque.push_back (octet);
            list.push_back (etag);
            etag.clear ();
            break;
        }
        if (1 & SHIFT[next_state][0])
            matched = true;
    }
    if (matched)
        std::swap (fields, list);
    return matched;
}

}//namespace http
