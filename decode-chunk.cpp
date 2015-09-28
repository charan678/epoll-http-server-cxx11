#include <string>
#include <cctype>
#include "http.hpp"
#include "decode-lookup-cls.hpp"

namespace http {

// S1: [0]  S2
//   | HEX  S3 A1{chunk-size << octet}
//
// S2: [0]  S2
//   | HEX  S3 A1{chunk-size << octet}
//   | [;]  S14
//   | [\r] S21
//
// S3: [0]  S3 A1{chunk-size << octet}
//   | HEX  S3 A1{chunk-size << octet}
//   | [;]  S4
//   | [\r] S11
//
// S4: tchar S5
// S5: tchar S5 | [;] S4 | [=] S6 | [\r] S11
// S6: tchar S7 | ["] S9
// S7: tchar S7 | [;] S4 | [\r] S11
// S8: qdtext S9 | [\\] S9 | ["] S9
// S9: qdtext S9 | [\\] S8 | ["] S10
// S10: [;] S4 | [\r] S11
// S11: [\n] S12
//
// S12: OCTET{chunk-size} S12 A2{body << octet}
//    | [\r] S13
//
// S13: [\n] S1
// S14: tchar S15
// S15: tchar S15 | [;] S14 | [=] S16 | [\r] S21
// S16: tchar S17 | ["] S19
// S17: tchar S17 | [;] S14 | [\r] S21
// S18: qdtext S19 | [\\] S19 | ["] S19
// S19: qdtext S19 | [\\] S18 | ["] S20
// S20: [;] S14 | [\r] S21
// S21: [\n] S22
// S22: tchar S23 | [\r] S27
// S23: tchar S23 | [:] S24
// S24: vchar S24 | [\t ] S24 | [\r] S25
// S25: [\n] S26
// S26: tchar S23 | [\t ] S24 | [\r] S27
// S27: [\n] S28
// S28: MATCH

decoder_chunk_type::decoder_chunk_type ()
: chunk_size_limit (CHUNK_SIZE_LIMIT), content_length (0),
  chunk_size (0), next_state (1) {}

void
decoder_chunk_type::clear ()
{
    content_length = 0;
    next_state = 1;
    chunk_size = 0;
}

bool
decoder_chunk_type::good () const
{
    return 28 == next_state;
}

bool
decoder_chunk_type::bad () const
{
    return 0 == next_state;
}

bool
decoder_chunk_type::partial () const
{
    return 1 <= next_state && next_state <= 27;
}

bool
decoder_chunk_type::put (int const octet, std::string& body)
{
    static const int SHIFT[29][13] = {
    //     vch tch  0  HEX  ;   =   :  WS  \\   "  \r  \n
        {0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
        {0, 0,  0,  2,  3,  0,  0,  0,  0,  0,  0,  0,  0}, // S1
        {0, 0,  0,  2,  3, 14,  0,  0,  0,  0,  0, 21,  0}, // S2
        {0, 0,  0,  3,  3,  4,  0,  0,  0,  0,  0, 11,  0}, // S3
        {0, 0,  5,  5,  5,  0,  0,  0,  0,  0,  0,  0,  0}, // S4
        {0, 0,  5,  5,  5,  4,  6,  0,  0,  0,  0, 11,  0}, // S5
        {0, 0,  7,  7,  7,  0,  0,  0,  0,  0,  9,  0,  0}, // S6
        {0, 0,  7,  7,  7,  4,  0,  0,  0,  0,  0, 11,  0}, // S7
        {0, 9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  0,  0}, // S8
        {0, 9,  9,  9,  9,  9,  9,  9,  9,  8, 10,  0,  0}, // S9
        {0, 0,  0,  0,  0,  4,  0,  0,  0,  0,  0, 11,  0}, // S10
        {0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 12}, // S11
        {0,12,  0,  0,  0,  0,  0,  0,  0,  0,  0, 13,  0}, // S12
        {0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1}, // S13
        {0, 0, 15, 15, 15,  0,  0,  0,  0,  0,  0,  0,  0}, // S14
        {0, 0, 15, 15, 15, 14, 16,  0,  0,  0,  0, 21,  0}, // S15
        {0, 0, 17, 17, 17,  0,  0,  0,  0,  0, 19,  0,  0}, // S16
        {0, 0, 17, 17, 17, 14,  0,  0,  0,  0,  0, 21,  0}, // S17
        {0,19, 19, 19, 19, 19, 19, 19, 19, 19, 19,  0,  0}, // S18
        {0,19, 19, 19, 19, 19, 19, 19, 19, 18, 20,  0,  0}, // S19
        {0, 0,  0,  0,  0, 14,  0,  0,  0,  0,  0, 21,  0}, // S20
        {0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 22}, // S21
        {0, 0, 23, 23, 23,  0,  0,  0,  0,  0,  0, 27,  0}, // S22
        {0, 0, 23, 23, 23,  0,  0, 24,  0,  0,  0,  0,  0}, // S23
        {0,24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 25,  0}, // S24
        {0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 26}, // S25
        {0, 0, 23, 23, 23,  0,  0,  0, 24,  0,  0, 27,  0}, // S26
        {0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 28}, // S27
        {1, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // S28
    };
    static const uint32_t CCLASS[16] = {
    //                 tn  r                          
        0x00000000, 0x08c00b00, 0x00000000, 0x00000000,
    //     !"#$%&'    ()*+,-./    01234567    89:;<=>?
        0x82a22222, 0x11221221, 0x34444444, 0x44751611,
    //    @ABCDEFG    HIJKLMNO    PQRSTUVW    XYZ[\]^_
        0x14444442, 0x22222222, 0x22222222, 0x22219122,
    //    `abcdefg    hijklmno    pqrstuvw    xyz{|}~ 
        0x24444442, 0x22222222, 0x22222222, 0x22212120,
    };
    if (! partial ())
        return false;
    int prev_state = next_state;
    int cls = lookup_cls (CCLASS, octet);
    if (12 == prev_state && --chunk_size >= 0)
        cls = 1;
    next_state = 0 == cls ? 0 : SHIFT[prev_state][cls];
    if (! next_state)
        return false;
    if (1 == prev_state)
        chunk_size = 0;
    if (3 == next_state) {
        if (chunk_size_limit <= chunk_size) {
            next_state = 0;
            return false;
        }
        chunk_size = chunk_size * 16
            + ( octet >= 'a' ? octet - 'a' + 10
              : octet >= 'A' ? octet - 'A' + 10
              : octet - '0');
    }
    if (12 == prev_state && 1 == cls) {
        body.push_back (octet);
        ++content_length;
    }
    return partial ();
}

}// namespace http
