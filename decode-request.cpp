#include <string>
#include <cctype>
#include "http.hpp"

namespace http {

//  S1: tchar  S2 A1{ req.method.push_back (octet); }
//
//  S2: tchar  S2 A1{ req.method.push_back (octet); }
//    | ' '    S3
//
//  S3: vchar  S4 A2{ req.uri.push_back (octet); }
//
//  S4: vchar  S4 A2{ req.uri.push_back (octet); }
//    | ' '    S5
//
//  S5: vchar  S6 A3{ req.http_version.push_back (octet); }
//
//  S6: vchar  S6 A3{ req.http_version.push_back (octet); }
//    | \r     S7
//
//  S7: \n     S8
//
//  S8: tchar  S9 A4{ name.push_back (std::tolower (octet)); }
//    | \r    S14
//
//  S9: tchar  S9 A4{ name.push_back (std::tolower (octet)); }
//    | ':'   S10
//
// S10: vchar S11 A5{ value.append (spaces); value.push_back (octet); }
//    | [\t ] S10
//    | \r    S12
//
// S11: vchar S11 A5{ value.append (spaces); value.push_back (octet); }
//    | [\t ] S11 A6{ spaces.push_back (c); }
//    | \r    S12
//
// S12: \n    S13
//
// S13: tchar  S9 A8{ req.header[name] = value; name.push_back (std::tolower (octet)); }
//    | [\t ] S10 A7{ spaces = " "; }
//    | \r    S14 A8{ req.header[name] = value; name.push_back (std::tolower (octet)); }
//
// S14: \n    S15
//
// S15: MATCH

decoder_request_type::decoder_request_type ()
    : next_state (1), name (), value (), nfield (0), nbyte (0),
      limit_nfield (100), limit_nbyte (8190)
{
}

void
decoder_request_type::clear ()
{
    next_state = 1;
    name.clear ();
    value.clear ();
    spaces.clear ();
    nfield = 0;
    nbyte = 0;
}

bool
decoder_request_type::good () const
{
    return 15 == next_state;
}

bool
decoder_request_type::bad () const
{
    return 0 == next_state;
}

bool
decoder_request_type::partial () const
{
    return 1 <= next_state && next_state <= 14;
}

bool
decoder_request_type::put (int const octet, request_type& req)
{
    static const int SHIFT[16][16] = {
    //      0  \d  \h  \T   .  \\   "   =   ;   ,   :  \t  \s  \r  \n
        {0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
        {0, 2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // S1
        {0, 2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,  3,  0,  0}, // S2
        {0, 4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  0,  0,  0,  0}, // S3
        {0, 4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  0,  5,  0,  0}, // S4
        {0, 6,  6,  6,  6,  6,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // S5
        {0, 6,  6,  6,  6,  6,  0,  0,  0,  0,  0,  0,  0,  0,  7,  0}, // S6
        {0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8}, // S7
        {0, 9,  9,  9,  9,  0,  0,  0,  0,  0,  0,  0,  0,  0, 14,  0}, // S8
        {0, 9,  9,  9,  9,  0,  0,  0,  0,  0,  0, 10,  0,  0,  0,  0}, // S9
        {0,11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 10, 10, 12,  0}, // S10
        {0,11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 12,  0}, // S11
        {0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 13}, // S12
        {0, 9,  9,  9,  9,  0,  0,  0,  0,  0,  0,  0, 10, 10, 14,  0}, // S13
        {0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 15}, // S14
        {1, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // S15
    };
    static const int ACTION[16][16] = {
    //      0  \d  \h  \T   .  \\   "   =   ;   ,   :  \t  \s  \r  \n
        {0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
        {0, 1,  1,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // S1
        {0, 1,  1,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // S2
        {0, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  0,  0,  0,  0}, // S3
        {0, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  0,  0,  0,  0}, // S4
        {0, 3,  3,  3,  3,  3,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // S5
        {0, 3,  3,  3,  3,  3,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // S6
        {0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // S7
        {0, 4,  4,  4,  4,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // S8
        {0, 4,  4,  4,  4,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // S9
        {0, 5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  0,  0,  0,  0}, // S10
        {0, 5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  6,  6,  0,  0}, // S11
        {0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // S12
        {0, 8,  8,  8,  8,  0,  0,  0,  0,  0,  0,  0,  7,  7,  8,  0}, // S13
        {0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // S14
        {0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // S15
    };
    if (! partial ())
        return false;
    if (nbyte++ > limit_nbyte) {
        next_state = 0;
        return false;
    }
    int cls = tocclass (octet);
    int prev_state = next_state;
    next_state = 0 == cls ? 0 : SHIFT[prev_state][cls];
    if (0 == next_state)
        return false;
    switch (ACTION[prev_state][cls]) {
    case 1:
        req.method.push_back (octet);           // case-sensitive (RFC 7230)
        break;
    case 2:
        req.uri.push_back (octet);
        break;
    case 3:
        req.http_version.push_back (octet);     // case-sensitive (RFC 7230)
        break;
    case 4:
        name.push_back (std::tolower (octet));  // case-insensitive (RFC 7230)
        break;
    case 5:
        value.append (spaces);
        value.push_back (octet);
        spaces.clear ();
        break;
    case 6:
        spaces.push_back (octet);
        break;
    case 7:
        spaces = " ";
        break;
    case 8:
        if (++nfield > limit_nfield) {
            next_state = 0;
            return false;
        }
        if (req.header.count (name) == 0)
            req.header[name] = value;
        else
            req.header[name] += "," + value;
        name.clear ();
        value.clear ();
        spaces.clear ();
        name.push_back (std::tolower (octet));  // case-insensitive (RFC 7230)
        nbyte = 0;
        break;
    }
    return partial ();
}

}//namespace http
