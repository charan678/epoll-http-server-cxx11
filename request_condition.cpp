#include <string>
#include <map>
#include <ctime>
#include "server.hpp"

int
request_condition_type::check (std::string const& method,
    std::map<std::string, std::string>& header)
{
    bool isget = (method == "GET" || method == "HEAD");
    int r = OK;
    if (header.count ("if-match") > 0)
        r = if_match (header.at ("if-match"));
    else if (header.count ("if-unmodified-since") > 0)
        r = if_unmodified_since (header.at ("if-unmodified-since"));
    if (OK != r)
        return BAD == r ? 400 : 412;
    if (header.count ("if-none-match") > 0)
        r = if_none_match (header.at ("if-none-match"));
    else if (isget && header.count ("if-modified-since") > 0)
        r = if_modified_since (header.at ("if-modified-since"));
    if (OK != r)
        return BAD == r ? 400 : isget ? 304 : 412;
    return 200;
}

int
request_condition_type::if_match (std::string const& field)
{
    std::vector<std::string> list;
    if (! decode_field (field, list))
        return BAD;
    if (list[0] == "*")
        return etag.empty () ? FAILED : OK;
    for (auto const& x : list)
        if (strong_compare (etag, x))
            return OK;
    return FAILED;
}

int
request_condition_type::if_none_match (std::string const& field)
{
    std::vector<std::string> list;
    if (! decode_field (field, list))
        return BAD;
    if (list[0] == "*")
        return etag.empty () ? OK : FAILED;
    for (auto const& x : list)
        if (weak_compare (etag, x))
            return FAILED;
    return OK;
}

int
request_condition_type::if_unmodified_since (std::string const& field)
{
    std::time_t since = decode_time ("%a, %d %b %Y %H:%M:%S GMT", field);
    return since < 0 ? BAD : (! etag.empty () && mtime <= since) ? OK : FAILED;
}

int
request_condition_type::if_modified_since (std::string const& field)
{
    std::time_t since = decode_time ("%a, %d %b %Y %H:%M:%S GMT", field);
    return since < 0 ? BAD : (etag.empty () || mtime > since) ? OK : FAILED;
}

bool
request_condition_type::strong_compare (std::string const& etag0, std::string const& etag1)
{
    return 'W' != etag0[0] && etag0 == etag1;
}

bool
request_condition_type::weak_compare (std::string const& etag0, std::string const& etag1)
{
    std::string::size_type s0 = 0;
    std::string::size_type s1 = 0;
    if ('W' == etag0[0])
        s0 += 2;
    if ('W' == etag1[0])
        s1 += 2;
    return etag0.compare (s0, etag0.size () - s0, etag1, s1, etag1.size () - s1) == 0;
}

bool
request_condition_type::decode_field (std::string const& field, std::vector<std::string>& list)
{
    static const int SHIFT[11][8] = {
        // \s   ,   *   W   /   "   .
        {0, 0,  0,  0,  0,  0,  0,  0},
        {0, 0,  2,  9,  3,  0,  5,  0}, // S1: ',' S2 | '*' S9 | 'W' S3 | '"' S5
        {0, 2,  2,  0,  3,  0,  5,  0}, // S2: \s S2 | ',' S2 | 'W' S3 | '"' S5
        {2, 0,  0,  0,  0,  4,  0,  0}, // S3: '/' S4
        {2, 0,  0,  0,  0,  0,  5,  0}, // S4: '"' S5
        {2, 0,  0,  5,  5,  5,  6,  5}, // S5: [\x21\x23-\x7e] S5 | '"' S6
        {7, 7,  8,  0,  0,  0,  0,  0}, // S6: \s S7 | ',' S8 | $
        {1, 7,  8,  0,  0,  0,  0,  0}, // S7: \s S7 | ',' S8 | $
        {1, 8,  8,  0,  3,  0,  5,  0}, // S8: \s S8 | ',' S8 | 'W' S3 | '"' S5 | $
        {7,10,  0,  0,  0,  0,  0,  0}, // S9: \s S10 | $
        {1,10,  0,  0,  0,  0,  0,  0}, // S10: \s S10 | $
    };
    list.clear ();
    std::string etag;
    int next_state = 1;
    for (int c : field) {
        int cls = '\t' == c ? 1 : ' ' == c ? 1 : ',' == c ? 2 : '*' == c ? 3
                : 'W' == c ? 4 : '/' == c ? 5 : '"' == c ? 6 : 0;
        int prev_state = next_state;
        if (5 == prev_state && 0 == cls)
            cls = (0x21 <= c && c <= 0x7e) ? 7 : 0;
        if (0 == cls || 0 == SHIFT[prev_state][cls])
            return false;
        next_state = SHIFT[prev_state][cls];
        if (2 & SHIFT[next_state][0])
            etag.push_back (c);
        if (4 & SHIFT[next_state][0]) {
            list.push_back (etag);
            etag.clear ();
        }
    }
    return (1 & SHIFT[next_state][0]);
}
