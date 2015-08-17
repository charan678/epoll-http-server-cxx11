#include <iostream>
#include <string>
#include <utility>
#include <cerrno>
#include <cstring>
#include "server.hpp"

logger_type::logger_type () {}
logger_type::~logger_type () {}
logger_type::logger_type(logger_type const&) {}
logger_type& logger_type::operator= (logger_type const&) { return *this; }

logger_type&
logger_type::getinstance ()
{
    static logger_type obj;
    return obj;
}

void
logger_type::put_error (std::string const& s)
{
    std::string e = std::strerror (errno);
    if (! s.empty ())
        e = s + ":" + e;
    std::string ts = to_string_time ("%a %b %e %H:%M:%S %Y");
    std::string t = "[" + ts + "] [error] " + e;
    std::cout << t << std::endl;
}

void
logger_type::put_info (std::string const& s)
{
    std::string ts = to_string_time ("%a %b %e %H:%M:%S %Y");
    std::string t = "[" + ts + "] [info] " + s;
    std::cout << t << std::endl;
}

void
logger_type::put_error (std::string const& ho, std::string const& s)
{
    std::string ts = to_string_time ("%a %b %e %H:%M:%S %Y");
    std::string t = "[" + ts + "] [error] [client " + ho +"] " + s;
    std::cout << t << std::endl;
}

static std::string
quote (std::string const& s)
{
    std::string t;
    for (int ch : s)
        switch (ch) {
        case '"': t += "\\\""; break;
        case '\r': t += "\\r"; break;
        case '\n': t += "\\n"; break;
        case '\t': t += "\\t"; break;
        default:
            if (0x20 < ch && ch < 0x7f)
                t.push_back (ch);
            else {
                int hi = (ch >> 4) & 0x0f;
                int lo = ch & 0x0f;
                t += "\\x";
                t.push_back (hi > 9 ? hi + ('a' - 10) : hi + '0');
                t.push_back (lo > 9 ? lo + ('a' - 10) : lo + '0');
            }
        }
    return std::move (t);
}

void
logger_type::put (std::string const& ho, request_type& req, response_type& res)
{
    std::string ts = to_string_time ("%d/%b/%Y:%H:%M:%S %z");
    std::string rl = "-";
    if (! req.method.empty ())
        rl = "\"" + quote (req.method)
            + " " + quote (req.uri)
            + " " + quote (req.http_version) + "\"";
    std::string t = ho + " - - [" + ts + "] " + rl + " "
        + std::to_string (res.code) + " " + std::to_string (res.content_length);
    std::cout << t << std::endl;
}
