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

decoder_request_line_type::decoder_request_line_type ()
    : limit_nbyte (8190), next_state (1), nbyte (0)
{
}

void
decoder_request_line_type::clear ()
{
    next_state = 1;
    nbyte = 0;
}

bool
decoder_request_line_type::good () const
{
    return 0x10 == next_state;
}

bool
decoder_request_line_type::bad () const
{
    return 0 == next_state;
}

bool
decoder_request_line_type::partial () const
{
    return 1 <= next_state && next_state <= 0x0f;
}

// request-line : method [ ] ([*] / origin-form) [ ] http-version CR LF
// method: tchar+
// origin-form : ([/] segment)+ ([?] query)?
// http-version : "HTTP/" [0-9] [.] [0-9]
// tchar : tpchar | tochar | [%]
// segment : (tpchar | pochar | [%] HEX HEX)*
// query : (tpchar | pochar | [/?] | [%] HEX HEX)*
// tpchar : [!$&'+\-.0-9A-Z_a-z~] | [*]
// pochar : [(),:;=@]
// tochar : [#^`|] | [%]

// [!$%&'*+\-.0-9A-Z_a-z~] 1 tchar pchar
// [#^`|] 2 tchar
// [(),:;=@/?] 3 pchar
// [*] 4 tchar pchar
// [/] 5 pchar
// [ ] 6

bool
decoder_request_line_type::put (uint32_t const octet, request_type& req)
{
    static const int SHIFT[14][11] = {
    //      tpchr tchr  pchr  [*]   [/]   [ ]
        {0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        {0, 0x22, 0x22, 0x00, 0x22, 0x00, 0x00}, // S1 tchar S2
        {0, 0x22, 0x22, 0x00, 0x22, 0x00, 0x03}, // S2 tchar S2 | [ ] S3
        {0, 0x00, 0x00, 0x00, 0x44, 0x45, 0x00}, // S3 [*] S4 | [/] S5
        {0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06}, // S4 [ ] S6
        {0, 0x45, 0x00, 0x45, 0x45, 0x45, 0x06}, // S5 pchar S5 | [ ] S6
        // [H] [T] [T] [P] [/] [0-9] [.] [0-9] [\r] [\n] MATCH
        // S6  S7  S8  S9  Sa  Sb    Sc  Sd    Se   Sf   S10
    };
    static const std::string HTTPVERSION ("HTTP/1.1\x0d\x0a");
    static const uint32_t CCLASS[16] = {
    //                 tn  r
        0x00000000, 0x00000000, 0x00000000, 0x00000000,
    //     !"#$%&'    ()*+,-./    01234567    89:;<=>?
        0x61021111, 0x33413115, 0x11111111, 0x11330303,
    //    @ABCDEFG    HIJKLMNO    PQRSTUVW    XYZ[\]^_
        0x31111111, 0x11111111, 0x11111111, 0x11100021,
    //    `abcdefg    hijklmno    pqrstuvw    xyz{|}~ 
        0x21111111, 0x11111111, 0x11111111, 0x11102010,
    };
    if (! partial ())
        return false;
    if (++nbyte > limit_nbyte)
        return failure ();
    int prev_state = next_state;
    if (prev_state <= 0x05) {
        std::wstring  wsegment;
        int cls = lookup_cls (CCLASS, octet);
        next_state = ! cls ? 0 : SHIFT[prev_state][cls] & 0x1f;
        switch (SHIFT[prev_state][cls] & 0xe0) {
        case 0x20:
            req.method.push_back (octet);
            break;
        case 0x40:
            req.uri.push_back (octet);
            break;
        }
    }
    else if (prev_state <= 0x0f) {
        uint32_t k = static_cast<uint8_t> (HTTPVERSION[prev_state - 0x06]);
        next_state = '1' == k && std::isdigit (octet) ? prev_state + 1
                   : k == octet ? prev_state + 1
                   : 0;
        if (' ' < k)
            req.http_version.push_back (octet);
    }
    return partial ();
}

}// namespace http
