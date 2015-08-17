#include <string>
#include <ctime>
#include <cctype>
#include "server.hpp"

static std::string
to_decimal (int const pad, int const width, int x)
{
    std::string t (width, pad);
    if (x == 0)
        t[width - 1] = '0';
    else
        for (int i = width; i > 0 && x != 0; --i) {
            t[i - 1] = (x % 10) + '0';
            x /= 10;
        }
    return t;
}

std::string
to_string_time (std::string const& fmt)
{
    return to_string_time (fmt, std::time (nullptr));
}

std::string
to_string_time (std::string const& fmt, std::time_t epoch)
{
    static const std::string a ("Sun Mon Tue Wed Thu Fri Sat ");
    static const std::string b ("Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec ");
    struct tm* p;
    if (fmt.find (" GMT") != std::string::npos
            || fmt.find (" UTC") != std::string::npos)
        p = std::gmtime (&epoch);
    else
        p = std::localtime (&epoch);
    struct tm dt = *p;
    char zone[8];
    char const zonefmt[] = "%z";
    std::strftime (zone, sizeof zone, zonefmt, &dt);
    std::string t;
    std::string::const_iterator s = fmt.cbegin ();
    while (s < fmt.cend ()) {
        int c = *s++;
        if (c != '%' || s >= fmt.cend ())
            t.push_back (c);
        else {
            c = *s++;
            switch (c) {
            default: t.push_back (c); break;
            case 'a': t += a.substr (dt.tm_wday * 4, 3); break;
            case 'b': t += b.substr (dt.tm_mon * 4, 3); break;
            case 'c': t += to_string_time ("%a %b %e %H:%M:%S %Y", epoch); break;
            case 'd': t += to_decimal ('0', 2, dt.tm_mday); break;
            case 'e': t += to_decimal (' ', 2, dt.tm_mday); break;
            case 'm': t += to_decimal ('0', 2, dt.tm_mon + 1); break;
            case 'z': t += zone; break;
            case 'F': t += to_string_time ("%Y-%m-%d", epoch); break;
            case 'H': t += to_decimal ('0', 2, dt.tm_hour); break;
            case 'M': t += to_decimal ('0', 2, dt.tm_min); break;
            case 'R': t += to_string_time ("%H:%M", epoch); break;
            case 'S': t += to_decimal ('0', 2, dt.tm_sec); break;
            case 'T': t += to_string_time ("%H:%M:%S", epoch); break;
            case 'Y': t += to_decimal ('0', 4, dt.tm_year + 1900); break;
            }
        }
    }
    return t;
}

static int
decode_timename (std::string::const_iterator& s, std::string::const_iterator const eos, std::string const& list)
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
decode_decimal (std::string::const_iterator& s, std::string::const_iterator const eos, std::size_t const n)
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
decode_time (std::string const& fmt, std::string const& datime)
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
            case 'a': dt.tm_wday = e = decode_timename (str, eos, a); break;
            case 'b': dt.tm_mon = e = decode_timename (str, eos, b); break;
            case 'Y': dt.tm_year = e = decode_decimal (str, eos, 4) - 1900; break;
            case 'm': dt.tm_mon = e = decode_decimal (str, eos, 2) - 1; break;
            case 'd': dt.tm_mday = e = decode_decimal (str, eos, 2); break;
            case 'H': dt.tm_hour = e = decode_decimal (str, eos, 2); break;
            case 'M': dt.tm_min = e = decode_decimal (str, eos, 2); break;
            case 'S': dt.tm_sec = e = decode_decimal (str, eos, 2); break;
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
