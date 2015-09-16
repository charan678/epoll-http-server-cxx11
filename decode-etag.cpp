#include <string>
#include "http.hpp"

namespace http {

bool
decode (std::vector<etag_type>& fields, std::string const& src)
{
    static const int SHIFT[10][9] = {
    //         \s   ,   W   /   *   "   .   $
        {   0,  0,  0,  0,  0,  0,  0,  0,  0},
        {   0,  1,  2,  3,  0,  8,  5,  0,  0}, // S1: \s S1 | ',' S2 | '*' S8 | '"' S5 | 'W' S3
        {   0,  2,  2,  3,  0,  0,  5,  0,  0}, // S2: \s S2 | ',' S2 | '"' S5 | 'W' S3
        {   0,  0,  0,  0,  4,  0,  0,  0,  0}, // S3: '/' S4
        {   0,  0,  0,  0,  0,  0,  5,  0,  0}, // S4: '"' S5
        {   0,  0,  0,  0,  0,  0,  6,  5,  0}, // S5: '"' S6 | [\x21\x23-\x7e] S5
        {   0,  6,  7,  0,  0,  0,  0,  0,  9}, // S6: \s S6 | ',' S7 | $ S9
        {   0,  7,  7,  3,  0,  0,  5,  0,  9}, // S7: \s S7 | ',' S7 | '"' S5 | 'W' S3 | $ S9
        {   0,  8,  0,  0,  0,  0,  0,  0,  9}, // S8: \s S8 | $ S9
        {   1,  0,  0,  0,  0,  0,  0,  0,  0}, // S9: MATCH
    };
    std::vector<etag_type> list;
    etag_type etag;
    int next_state = 1;
    std::string::const_iterator s = src.cbegin ();
    std::string::const_iterator const e = src.cend ();
    for (; s <= e; ++s) {
        int ch = s == e ? '\0' : *s;
        int cls = s == e ? 8
                : '\t' == ch ? 1 : ' ' == ch ? 1
                : '"' == ch ? 6
                : (5 == next_state && 0x21 <= ch && ch <= 0x7e) ? 7
                : ',' == ch ? 2 : '*' == ch ? 5
                : 'W' == ch ? 3 : '/' == ch ? 4 : 0;
        int prev_state = next_state;
        next_state = cls ? SHIFT[prev_state][cls] : 0;
        if (! next_state)
            break;
        if (3 == cls)
            etag.weak = true;
        if (5 <= cls && cls <= 7)
            etag.opaque.push_back (ch);
        else if ((6 == prev_state || 8 == prev_state) && (2 == cls || 8 == cls)) {
            list.push_back (etag);
            etag.clear ();
        }
    }
    if (! (1 & SHIFT[next_state][0]))
        return false;
    std::swap (fields, list);
    return true;
}

}//namespace http
