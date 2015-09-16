#include <string>
#include <ctime>
#include <cctype>
#include "http.hpp"

namespace http {

static int
time_decode_name (std::string::const_iterator& s, std::string::const_iterator const eos, std::string const& list)
{
    std::string::const_iterator const s0 = s;
    for (std::size_t i = 0; i < 3; ++i)
        if (s >= eos || ! std::isalpha (*s++))
            return -1;
    std::string name (s0, s);
    name.push_back (' ');
    std::string::size_type i = list.find (name);
    if (i == std::string::npos)
        return -1;
    return i / 4;
}

static int
time_decode_decimal (std::string::const_iterator& s, std::string::const_iterator const eos, std::size_t const n)
{
    int v = 0;
    for (std::size_t i = 0; i < n; ++i) {
        if (s >= eos || ! std::isdigit (*s))
            return -1;
        v = v * 10 + *s++ - '0';
    }
    return v;
}

std::time_t
time_decode (std::string const& fmt, std::string const& datime)
{
    struct tm dt;
    static const std::string a ("Sun Mon Tue Wed Thu Fri Sat ");
    static const std::string b ("Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec ");
    std::string::const_iterator pat = fmt.cbegin ();
    std::string::const_iterator const eot = fmt.cend ();
    std::string::const_iterator str = datime.cbegin ();
    std::string::const_iterator const eos = datime.cend ();
    while (pat < eot) {
        int c = *pat++;
        if ('%' != c) {
            if (str >= eos || c != *str++)
                return -1;
        }
        else if (pat < eot) {
            c = *pat++;
            int e = 0;
            switch (c) {
            case 'a': dt.tm_wday = e = time_decode_name (str, eos, a); break;
            case 'b': dt.tm_mon = e = time_decode_name (str, eos, b); break;
            case 'Y': dt.tm_year = e = time_decode_decimal (str, eos, 4) - 1900; break;
            case 'm': dt.tm_mon = e = time_decode_decimal (str, eos, 2) - 1; break;
            case 'd': dt.tm_mday = e = time_decode_decimal (str, eos, 2); break;
            case 'H': dt.tm_hour = e = time_decode_decimal (str, eos, 2); break;
            case 'M': dt.tm_min = e = time_decode_decimal (str, eos, 2); break;
            case 'S': dt.tm_sec = e = time_decode_decimal (str, eos, 2); break;
            default:
                if (str >= eos || c != *str++)
                    return -1;
            }
            if (e < 0)
                return -1;
        }
    }
    if (datime.find ("GMT") != std::string::npos
            || datime.find ("UTC") != std::string::npos)
        return timegm (&dt);
    return timelocal (&dt);
}

}//namespace http
