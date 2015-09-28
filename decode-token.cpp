#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include "http.hpp"

namespace http {

// RFC 7230 HTTP/1.1
//
//      fields-value: [,\s]* token (\s* ';' \s* parameter)*
//                   (\s* ',' (\s* token (\s* ';' \s* parameter)*)?)* \s*
//      parameter: token \s* '=' \s* (token | '"' ('\\' qdesc | qdtext)* '"'
//      token: tchar+
//      tchar: [A-Za-z0-9!#$%&'*+\-.^_`|~]
//      qdesc: qdtext | '\\' | '"'
//      qdtext: [\t \x21\x23-\x5b\x5d-\x7e]
//
//      S1: tchar   S2  A1{ item.token.push_back (octet); }
//        | [,]     S1
//        | [\t ]   S1
//
//      S2: tchar   S2  A1{ item.token.push_back (octet); }
//        | [\t ]   S3
//        | [;]     S4
//        | [,]     Sc  A5{ fields.push_back (item); }
//        | $       Sd  A5{ fields.push_back (item); }
//
//      S3: [\t ]   S3
//        | [;]     S4
//        | [,]     Sc  A5{ fields.push_back (item); }
//        | $       Sd  A5{ fields.push_back (item); }
//
//      S4: tchar   S5  A2{ name.push_back (std::tolower (octet)); }
//        | [\t ]   S4
//
//      S5: tchar   S5  A2{ name.push_back (std::tolower (octet)); }
//        | [\t ]   S6
//        | [=]     S7
//
//      S6: [\t ]   S6
//        | [=]     S7
//
//      S7: tchar   S8  A3{ value.push_back (octet); }
//        | [\t ]   S7
//        | ["]     Sa
//
//      S8: tchar   S8  A3{ value.push_back (octet); }
//        | [\t ]   Sb  A4{ item.parameter.push_back (name, value); }
//        | [;]     S4  A4{ item.parameter.push_back (name, value); }
//        | [,]     Sc  A6{ A4 (); A5 (); }
//        | $       Sd  A6{ A4 (); A5 (); }
//
//      S9: qdtext  Sa  A3{ value.push_back (octet); }
//        | [\\]    Sa  A3{ value.push_back (octet); }
//        | ["]     Sa  A3{ value.push_back (octet); }
//
//      Sa: qdtext  Sa  A3{ value.push_back (octet); }
//        | [\\]    S9
//        | ["]     Sb  A4{ item.parameter.push_back (name, value); }
//
//      Sb: [\t ]   Sb
//        | [;]     S4
//        | [,]     Sc  A5{ fields.push_back (item); }
//        | $       Sd  A5{ fields.push_back (item); }
//
//      Sc: tchar   S2  A1{ item.token.push_back (octet); }
//        | [\t ]   Sc
//        | [,]     Sc
//        | $       Sd
//
//      Sd: MATCH

static inline int
lookup_cls (uint32_t const tbl[], uint32_t const octet)
{
    uint32_t const i = octet >> 3;
    uint32_t const count = (7 - (octet & 7)) << 2;
    return octet < 128 ? ((tbl[i] >> count) & 0x0f) : 0;
}

bool
decode (std::vector<token_type>& fields, std::string const& src, int const lowerlimit)
{
    static const int SHIFT[14][10] = {
    //     qdtext tchar [\t ] [;]   [=]   [,]   [\\]  ["]   $
        {0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        {0, 0x00, 0x12, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00}, // S1
        {0, 0x00, 0x12, 0x03, 0x04, 0x00, 0x5c, 0x00, 0x00, 0x5d}, // S2
        {0, 0x00, 0x00, 0x03, 0x04, 0x00, 0x5c, 0x00, 0x00, 0x5d}, // S3
        {0, 0x00, 0x25, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // S4
        {0, 0x00, 0x25, 0x06, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00}, // S5
        {0, 0x00, 0x00, 0x06, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00}, // S6
        {0, 0x00, 0x38, 0x07, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x00}, // S7
        {0, 0x00, 0x38, 0x4b, 0x44, 0x00, 0x6c, 0x00, 0x00, 0x6d}, // S8
        {0, 0x3a, 0x3a, 0x3a, 0x3a, 0x3a, 0x3a, 0x3a, 0x3a, 0x00}, // S9
        {0, 0x3a, 0x3a, 0x3a, 0x3a, 0x3a, 0x3a, 0x09, 0x4b, 0x00}, // Sa
        {0, 0x00, 0x00, 0x0b, 0x04, 0x00, 0x5c, 0x00, 0x00, 0x5d}, // Sb
        {0, 0x00, 0x12, 0x0c, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x0d}, // Sc
        {1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // Sd
    };
    static const uint32_t CCLASS[16] = {
    //                 tn  r                          
        0x00000000, 0x03000000, 0x00000000, 0x00000000,
    //     !"#$%&'    ()*+,-./    01234567    89:;<=>?
        0x32822222, 0x11226221, 0x22222222, 0x22141511,
    //    @ABCDEFG    HIJKLMNO    PQRSTUVW    XYZ[\]^_
        0x12222222, 0x22222222, 0x22222222, 0x22217122,
    //    `abcdefg    hijklmno    pqrstuvw    xyz{|}~ 
        0x22222222, 0x22222222, 0x22222222, 0x22212120,
    };
    std::vector<token_type> list;
    token_type item;
    std::string::const_iterator s = src.cbegin ();
    std::string::const_iterator const e = src.cend ();
    std::string name;
    std::string value;
    bool matched = false;
    int next_state = 1 == lowerlimit ? 1 : 12;
    for (; s <= e; ++s) {
        uint32_t octet = s == e ? '\0' : static_cast<uint8_t> (*s);
        int cls = s == e ? 9 : lookup_cls (CCLASS, octet);
        int prev_state = next_state;
        next_state = 0 == cls ? 0 : SHIFT[prev_state][cls] & 0x0f;
        if (! next_state)
            break;
        switch (SHIFT[prev_state][cls] & 0xf0) {
        case 0x10:
            item.token.push_back (octet);
            break;
        case 0x20:
            name.push_back (std::tolower (octet));
            break;
        case 0x30:
            value.push_back (octet);
            break;
        case 0x40:
            item.parameter.push_back (name);
            item.parameter.push_back (value);
            name.clear ();
            value.clear ();
            break;
        case 0x50:
            list.push_back (item);
            item.clear ();
            break;
        case 0x60:
            item.parameter.push_back (name);
            item.parameter.push_back (value);
            list.push_back (item);
            name.clear ();
            value.clear ();
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
