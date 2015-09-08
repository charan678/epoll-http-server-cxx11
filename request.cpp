#include <cctype>
#include "server.hpp"

// see `RFC 7230 HTTP/1.1 Message Syntax and Routing'

static int
tocclass (int c)
{
    // 1:CR 2:LF 3:TAB 4:SP 5:':' 6:'=' 7:'"' 8:'\\' 9:';' 10:','
    // 11:'0' 12:HEXDIGIT 13:tchar 14:vchar
    static int CCLASS[128] = {
    //  0   1   2   3   4   5   6   7   8   9   a   b   c   d   e   f
        0,  0,  0,  0,  0,  0,  0,  0,  0,  3,  2,  0,  0,  1,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    //      !   "   #   $   %   &   '   (   )   *   +   ,   -   .   /
        4, 13,  7, 13, 13, 13, 13, 13, 14, 14, 13, 13, 10, 13, 13, 14,
    //  0   1   2   3   4   5   6   7   8   9   :   ;   <   =   >   ?
       11, 12, 12, 12, 12, 12, 12, 12, 12, 12,  5,  9, 14,  6, 14, 14,
    //  @   A   B   C   D   E   F   G   H   I   J   K   L   M   N   O
       14, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    //  P   Q   R   S   T   U   V   W   X   Y   Z   [   \   ]   ^   _
       13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 14,  8, 14, 13, 13,
    //  `   a   b   c   d   e   f   g   h   i   j   k   l   m   n   o
       13, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    //  p   q   r   s   t   u   v   w   x   y   z   {   |   }   ~
       13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 14, 13, 14, 13,  0,
    };
    return (0 <= c && c <= 127) ? CCLASS[c] : c > 127 ? 14 : 0; 
}

bool
request_type::header_token (std::string const& str, std::vector<std::string>& tokens)
{
    static int SHIFT[14][15] = {
        // CR  LF TAB  SP   :   =   "   \   ;   ,   0 HEX tch vch
        {0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
        {0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  2,  3,  3,  3,  0}, // S1: ',' S2 | tchar S3
        {0, 0,  0,  2,  2,  0,  0,  0,  0,  0,  2,  3,  3,  3,  0}, // S2: TAB S2 | SP S2 | ',' S2 | tchar S3
        {3, 0,  0,  4,  4,  0,  0,  0,  0,  5, 13,  3,  3,  3,  0}, // S3: TAB S4 | SP S4 | ';' S5 | ',' S13 | tchar S3 | $
        {1, 0,  0,  4,  4,  0,  0,  0,  0,  5, 13,  0,  0,  0,  0}, // S4: TAB S4 | SP S4 | ';' S5 | ',' S13 | $
        {0, 0,  0,  5,  5,  0,  0,  0,  0,  0,  0,  6,  6,  6,  0}, // S5: TAB S5 | SP S5 | tchar S6
        {2, 0,  0,  7,  7,  0,  8,  0,  0,  0,  0,  6,  6,  6,  0}, // S6: TAB S7 | SP S7 | '=' S8 | tchar S6
        {0, 0,  0,  7,  7,  0,  8,  0,  0,  0,  0,  0,  0,  0,  0}, // S7: TAB S7 | SP S7 | '=' S8
        {0, 0,  0,  8,  8,  0,  0, 10,  0,  0,  0,  9,  9,  9,  0}, // S8: TAB S8 | SP S8 | '"' S10 | tchar S9
        {5, 0,  0,  4,  4,  0,  0,  0,  0,  5, 13,  9,  9,  9,  0}, // S9: TAB S4 | SP S4 | ';' S5 | ',' S13 | tchar S9 | $
        {4, 0,  0, 10, 10, 10, 10, 12, 11, 10, 10, 10, 10, 10, 10}, // S10: TAB S10 | SP S10 | '"' S12 | '\\' S11 | VCHAR S10
        {4, 0,  0, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10}, // S11: TAB S10 | SP S10 | VCHAR S10
        {5, 0,  0,  4,  4,  0,  0,  0,  0,  5, 13,  0,  0,  0,  0}, // S12: TAB S4 | SP S4 | ';' S5 | ',' S13 | $
        {1, 0,  0, 13, 13,  0,  0,  0,  0,  0, 13,  3,  3,  3,  0}, // S13: TAB S13 | SP S13 | ',' S13 | tchar S3 | $
    };
    tokens.clear ();
    std::string value;
    int next_state = 1;
    for (int c : str) {
        int cls = tocclass (c);
        int prev_state = next_state;
        if ((10 == prev_state || 11 == prev_state) && 0 == cls)
            cls = ' ' <= c && 0x7f != c ? 7 : 0;
        next_state = 0 == cls ? 0 : SHIFT[prev_state][cls];
        if (0 == next_state)
            return false;
        if (2 & SHIFT[next_state][0])
            value.push_back (std::tolower (c));
        if (4 & SHIFT[next_state][0])
            value.push_back (c);
        if (3 == prev_state && 3 != next_state) {
            tokens.push_back (value);
            value.clear ();
        }
        else if (6 == prev_state && 6 != next_state) {
            tokens.back () += ";" + value;
            value.clear ();
        }
        else if ((9 == prev_state && 9 != next_state) || 12 == next_state) {
            tokens.back () += "=" + value;
            value.clear ();
        }
    }
    if (3 == next_state)
        tokens.push_back (value);
    else if (9 == next_state)
        tokens.back () += "=" + value;
    return (1 & SHIFT[next_state][0]) != 0;
}


ssize_t
request_type::canonical_length (std::string const& s)
{
    static const int SHIFT[6][4] = {
        {0, 0,  0,  0},
        {0, 0,  2,  3}, // S1: ',' S2 | \d S3
        {0, 2,  2,  3}, // S2: \s S2 | ',' S2 | \d S3
        {1, 4,  5,  3}, // S3: \s S4 | ',' S5 | \d S3
        {1, 4,  5,  0}, // S4: \s S4 | ',' S5
        {1, 5,  5,  3}, // S5: \s S5 | ',' S5 | \d S3
    };
    ssize_t prev_value = -1;
    ssize_t value = 0;
    int next_state = 1;
    for (int c : s) {
        int cls = '\t' == c ? 1 : ' ' == c ? 1 : ',' == c ? 2
                : std::isdigit ('0') ? 3 : 0;
        int prev_state = next_state;
        next_state = cls == 0 ? 0 : SHIFT[prev_state][cls];
        if (! next_state)
            return -400;
        if (3 == prev_state && 3 != next_state) {
            if (prev_value >= 0 && prev_value != value)
                return -400;
            prev_value = value;
        }
        if (3 == next_state) {
            if (3 != prev_state)
                value = 0;
            // 214748364L * 10 + 7 == 0x7fffffffL
            if (value > 214748364L || (value == 214748364L && c > '7'))
                return -413;
            value = value * 10 + c - '0';
        }
    }
    if (! (1 & SHIFT[next_state][0]))
        return -400;
    if (prev_value >= 0 && prev_value != value)
        return -400;
    return value;
}

void
request_type::clear ()
{
    method.clear ();
    uri.clear ();
    http_version.clear ();
    header.clear ();
    content_length = 0;
    body.clear ();
}

request_decoder_type::request_decoder_type ()
: next_state (1), name (), value (), nfield (0), nbyte (0) {}

void
request_decoder_type::clear ()
{
    next_state = 1;
    name.clear ();
    value.clear ();
    spaces.clear ();
    nfield = 0;
    nbyte = 0;
}

bool
request_decoder_type::good () const
{
    return 17 == next_state;
}

bool
request_decoder_type::bad () const
{
    return 0 == next_state;
}

bool
request_decoder_type::partial () const
{
    return 1 <= next_state && next_state <= 16;
}

bool
request_decoder_type::put (int const c, request_type& req)
{
    static const int SHIFT[18][15] = {
        // CR  LF TAB  SP   :   =   "   \   ;   ,   0 HEX tch vch
        {0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
        {0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  2,  2,  2,  0}, // S1: tchar S2
        {0, 0,  0,  0,  3,  0,  0,  0,  0,  0,  0,  2,  2,  2,  0}, // S2: SP S3 | tchar S2
        {0, 0,  0,  0,  0,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4}, // S3: vchar S4
        {0, 0,  0,  0,  5,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4}, // S4: SP S5 | vchar S4
        {0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  6,  6,  6,  6}, // S5: vchar S6
        {0, 7,  0,  0,  0,  0,  0,  0,  0,  0,  0,  6,  6,  6,  6}, // S6: CR S7 | vchar S6
        {0, 0,  8,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // S7: LF S8
        {0,16,  0,  0,  0,  0,  0,  0,  0,  0,  0,  9,  9,  9,  0}, // S8: CR S16 | tchar S9
        {2, 0,  0,  0,  0, 10,  0,  0,  0,  0,  0,  9,  9,  9,  0}, // S9: ':' S10 | tchar S9
        {0,13,  0, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11}, // S10: CR S13 | [\t ] S10 | vchar S11
        {0,13,  0, 12, 12, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11}, // S11: CR S13 | [\t ] S12 | vchar S11
        {0,13,  0, 12, 12, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11}, // S12: CR S13 | [\t ] S12 | vchar S11
        {0, 0, 14,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // S13: LF S14
        {0,16,  0, 15, 15,  0,  0,  0,  0,  0,  0,  9,  9,  9,  0}, // S14: CR S16 | [\t ] S15 | tchar S9
        {0,13,  0, 15, 15, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11}, // S15: CR S13 | [\t ] S15 | vchar S11
        {2, 0, 17,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // S16: LF S17
        {1, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // S17: MATCH
    };
    if (! partial ())
        return false;
    if (nbyte++ > LIMIT_REQUEST_FIELD_SIZE) {
        next_state = 0;
        return false;
    }
    int cls = tocclass (c);
    int prev_state = next_state;
    next_state = 0 == cls ? 0 : SHIFT[prev_state][cls];
    if (0 == next_state)
        return false;
    if ((14 == prev_state) && (2 & SHIFT[next_state][0])) {
        if (req.header.count (name) == 0)
            req.header[name] = value;
        else
            req.header[name] += "," + value;
        name.clear ();
        value.clear ();
        spaces.clear ();
        nbyte = 0;
        if (nfield++ > LIMIT_REQUEST_FIELDS) {
            next_state = 0;
            return false;
        }
    }
    if (11 == next_state) {
        if (12 == prev_state) {
            value.append (spaces);
            spaces.clear ();
        }
        else if (15 == prev_state) {
            value.push_back (' ');
            spaces.clear ();
        }
        value.push_back (c);
    }
    else if (12 == next_state)
        spaces.push_back (c);
    else if (9 == next_state)
        name.push_back (std::tolower (c));  // case-insensitive (RFC 7230)
    else if (2 == next_state)
        req.method.push_back (c);           // case-sensitive (RFC 7230)
    else if (4 == next_state)
        req.uri.push_back (c);
    else if (6 == next_state)
        req.http_version.push_back (c);     // case-sensitive (RFC 7230)
    return partial ();
}

chunk_decoder_type::chunk_decoder_type ()
: chunk_size_limit (CHUNK_SIZE_LIMIT), content_length (0),
  chunk_size (0), next_state (1) {}

void
chunk_decoder_type::clear ()
{
    content_length = 0;
    next_state = 1;
    chunk_size = 0;
}

bool
chunk_decoder_type::good () const
{
    return 28 == next_state;
}

bool
chunk_decoder_type::bad () const
{
    return 0 == next_state;
}

bool
chunk_decoder_type::partial () const
{
    return 1 <= next_state && next_state <= 27;
}

bool
chunk_decoder_type::put (int const c, std::string& body)
{
    static const int SHIFT[29][15] = {
        // CR  LF TAB  SP   :   =   "   \   ;   ,   0 HEX tch VCH
        {0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
        {4, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  2,  3,  0,  0}, // S1: '0' S2 | HEX S3
        {0,21,  0,  0,  0,  0,  0,  0,  0, 14,  0,  2,  3,  0,  0}, // S2: CR S21 | ';' S14 | '0' S2 | HEX S3
        {2,11,  0,  0,  0,  0,  0,  0,  0,  4,  0,  3,  3,  0,  0}, // S3: CR S11 | ';' S4 | '0' S3 | HEX S3
        {0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  5,  5,  5,  0}, // S4: tchar S5
        {0,11,  0,  0,  0,  0,  6,  0,  0,  4,  0,  5,  5,  5,  0}, // S5: CR S11 | '=' S6 | ';' S4 | tchar S5
        {0, 0,  0,  0,  0,  0,  0,  8,  0,  0,  0,  7,  7,  7,  0}, // S6: '"' S8 | tchar S7
        {0,11,  0,  0,  0,  0,  0,  0,  0,  4,  0,  7,  7,  7,  0}, // S7: CR S11 | ';' S4 | tchar S7
        {0, 0,  0,  8,  8,  8,  8, 10,  9,  8,  8,  8,  8,  8,  8}, // S8: TAB S8 | SP S8 | '"' S10 | '\\' S9 | VCHAR S8
        {0, 0,  0,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8}, // S9: TAB S8 | SP S8 | VCHAR S8
        {0,11,  0,  0,  0,  0,  0,  0,  0,  4,  0,  0,  0,  0,  0}, // S10: CR S11 | ';' S4
        {0, 0, 12,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // S11: LF S12
        {8,13, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12}, // S12: OCTET{chunk-size} CR S13
        {0, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // S13: LF S1
        {0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 15, 15, 15,  0}, // S14: tchar S15
        {0,21,  0,  0,  0,  0, 16,  0,  0, 14,  0, 15, 15, 15,  0}, // S15: CR S21 | '=' S16 | ';' S14 | tchar S15
        {0, 0,  0,  0,  0,  0,  0, 18,  0,  0,  0, 17, 17, 17,  0}, // S16: '"' S18 | tchar S17
        {0,21,  0,  0,  0,  0,  0,  0,  0, 14,  0, 17, 17, 17,  0}, // S17: CR S21 | ';' S14 | tchar S17
        {0, 0,  0, 18, 18, 18, 18, 20, 19, 18, 18, 18, 18, 18, 18}, // S18: TAB S18 | SP S18 | '"' S20 | '\\' S19 | VCHAR S18
        {0, 0,  0, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18}, // S19: TAB S18 | SP S18 | VCHAR S18
        {0,21,  0,  0,  0,  0,  0,  0,  0, 14,  0,  0,  0,  0,  0}, // S20: CR S21 | ';' S14
        {0, 0, 22,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // S21: LF S22
        {0,27,  0,  0,  0,  0,  0,  0,  0,  0,  0, 23, 23, 23,  0}, // S22: CR S27 | tchar S23
        {0,27,  0,  0,  0, 24,  0,  0,  0,  0,  0, 23, 23, 23,  0}, // S23: ':' S24 | tchar S23
        {0,25,  0, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24}, // S24: CR S25 | TAB S24 | SP S24 | VCHAR S24
        {0, 0, 26,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // S25: LF S26
        {0,27,  0, 24, 24,  0,  0,  0,  0,  0,  0, 23, 23, 23,  0}, // S26: CR S27 | TAB S24 | SP S24 | tchar S23
        {0, 0, 28,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // S27: LF S28
        {1, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // S28: MATCH
    };
    if (! partial ())
        return false;
    int prev_state = next_state;
    if (4 & SHIFT[prev_state][0])
        chunk_size = 0;
    int cls = tocclass (c);
    if ((8 & SHIFT[prev_state][0]) && --chunk_size >= 0)
        cls = 14;
    next_state = 0 == cls ? 0 : SHIFT[prev_state][cls];
    if (0 == next_state)
        return false;
    if (2 & SHIFT[next_state][0]) {
        if (chunk_size_limit <= chunk_size) {
            next_state = 0;
            return false;
        }
        chunk_size = chunk_size * 16
            + (c >= 'a' ? c - 'a' + 10 : c >= 'A' ? c - 'A' + 10 : c - '0');
    }
    if ((8 & SHIFT[prev_state][0]) && (8 & SHIFT[next_state][0])) {
        body.push_back (c);
        ++content_length;
    }
    return partial ();
}
