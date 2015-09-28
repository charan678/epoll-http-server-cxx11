#include <string>
#include <cctype>
#include "http.hpp"

namespace http {

static inline int
lookup_cls (uint32_t const tbl[], uint32_t const octet)
{
    uint32_t const i = octet >> 3;
    uint32_t const count = (7 - (octet & 7)) << 2;
    return octet < 128 ? ((tbl[i] >> count) & 0x0f) : 0;
}

decoder_request_header_type::decoder_request_header_type ()
    : next_state (1), name (), value (), nfield (0), nbyte (0),
      limit_nfield (100), limit_nbyte (8190)
{
}

void
decoder_request_header_type::clear ()
{
    next_state = 1;
    name.clear ();
    value.clear ();
    spaces.clear ();
    nfield = 0;
    nbyte = 0;
}

bool
decoder_request_header_type::good () const
{
    return 8 == next_state;
}

bool
decoder_request_header_type::bad () const
{
    return 0 == next_state;
}

bool
decoder_request_header_type::partial () const
{
    return 1 <= next_state && next_state <= 7;
}

// S1: tchar S2 | [\r] S7
// S2: tchar S2 | [:] S3
// S3: vchar S4 | [\t ] S3 | [\r] S5
// S4: vchar S4 | [\t ] S4 | [\r] S5
// S5: [\n] S6
// S6: tchar S2 | [\t ] S3 | [\r] S7
// S7: [\n] S8
// S8: MATCH

bool
decoder_request_header_type::put (uint32_t const octet, request_type& req)
{
    static const int SHIFT[9][7] = {
    //      vch   [:]   tchar [\t ] [\r]  [\n]
        {0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        {0, 0x00, 0x00, 0x12, 0x00, 0x07, 0x00}, // S1: tchar S2 | [\r] S7
        {0, 0x00, 0x03, 0x12, 0x00, 0x00, 0x00}, // S2: tchar S2 | [:] S3
        {0, 0x24, 0x24, 0x24, 0x03, 0x05, 0x00}, // S3: vchar S4 | [\t ] S3 | [\r] S5
        {0, 0x24, 0x24, 0x24, 0x34, 0x05, 0x00}, // S4: vchar S4 | [\t ] S4 | [\r] S5
        {0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06}, // S5: [\n] S6
        {0, 0x02, 0x02, 0x52, 0x43, 0x57, 0x00}, // S6: vchar S2 | [\t ] S3 | [\r] S7
        {0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08}, // S7: [\n] S8
        {0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // S8: MATCH
    };
    static const uint32_t CCLASS[16] = {
    //                 tn  r                          
        0x00000000, 0x04600500, 0x00000000, 0x00000000,
    //     !"#$%&'    ()*+,-./    01234567    89:;<=>?
        0x43133333, 0x11331331, 0x33333333, 0x33211111,
    //    @ABCDEFG    HIJKLMNO    PQRSTUVW    XYZ[\]^_
        0x13333333, 0x33333333, 0x33333333, 0x33311133,
    //    `abcdefg    hijklmno    pqrstuvw    xyz{|}~ 
        0x33333333, 0x33333333, 0x33333333, 0x33313130,
    };
    if (! partial ())
        return false;
    if (++nbyte > limit_nbyte)
        return failure ();
    int cls = lookup_cls (CCLASS, octet);
    int prev_state = next_state;
    next_state = ! cls ? 0 : SHIFT[prev_state][cls] & 0x0f;
    if (! next_state)
        return false;
    switch (SHIFT[prev_state][cls] & 0xf0) {
    case 0x10:
        name.push_back (std::tolower (octet));
        break;
    case 0x20:
        value.append (spaces);
        value.push_back (octet);
        spaces.clear ();
        break;
    case 0x30:
        spaces.push_back (octet);
        break;
    case 0x40:
        spaces = " ";
        break;
    case 0x50:
        if (++nfield > limit_nfield)
            return failure ();
        if (req.header.count (name) == 0)
            req.header[name] = value;
        else
            req.header[name] += "," + value;
        name.clear ();
        value.clear ();
        spaces.clear ();
        name.push_back (std::tolower (octet));
        nbyte = 0;
        break;
    }
    return partial ();
}

}// namespace http
