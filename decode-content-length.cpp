#include <string>
#include <cctype>
#include "http.hpp"
#include "decode-lookup-cls.hpp"

namespace http {

// S1: [0-9] S2 | [,] S1 | [\t ] S1
// S2: [0-9] S2 | [,] S4 | [\t ] S3 | $ S5
// S3: [,] S4 | [\t ] S3 | $ S5
// S4: [0-9] S2 | [,] S4 | [\t ] S4 | $ S5
// S5: MATCH

bool
decode (content_length_type& field, std::string const& src)
{
    static const int8_t SHIFT[6][5] = {
    //      [0-9] [,]   [\t ] $
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
        0x30000000, 0x00002000, 0x11111111, 0x11000000,
    //    @ABCDEFG    HIJKLMNO    PQRSTUVW    XYZ[\]^_
        0x00000000, 0x00000000, 0x00000000, 0x00000000,
    //    `abcdefg    hijklmno    pqrstuvw    xyz{|}~ 
        0x00000000, 0x00000000, 0x00000000, 0x00000000,
    };
    std::string::const_iterator s = src.cbegin ();
    std::string::const_iterator const e = src.cend ();
    ssize_t length = -1;
    ssize_t value = 0;
    field.status = 400;
    field.length = 0;
    bool matched = false;
    for (int next_state = 1; s <= e; ++s) {
        uint32_t octet = s == e ? '\0' : static_cast<uint8_t> (*s);
        int cls = s == e ? 4 : lookup_cls (CCLASS, octet);
        int prev_state = next_state;
        next_state = ! cls ? 0 : SHIFT[prev_state][cls] & 0x0f;
        if (! next_state)
            break;
        switch (SHIFT[prev_state][cls] & 0xf0) {
        case 0x10:
            // 214748364L * 10 + 7 == 0x7fffffffL
            if (value > 214748364L || (value == 214748364L && octet > '7')) {
                field.status = 413;
                return false;
            }
            value = value * 10 + octet - '0';
            break;
        case 0x20:
            if (length >= 0 && length != value)
                return false;
            length = value;
            value = 0;
            break;
        }
        if (1 & SHIFT[next_state][0]) {
            field.status = 200;
            field.length = length;
            matched = true;
        }
    }
    return matched;
}

}//namespace http
