#include <string>
#include <ctime>
#include <cctype>
#include "http.hpp"

namespace http {

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
time_to_string (std::string const& fmt)
{
    return time_to_string (fmt, std::time (nullptr));
}

std::string
time_to_string (std::string const& fmt, std::time_t epoch)
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
            case 'c': t += time_to_string ("%a %b %e %H:%M:%S %Y", epoch); break;
            case 'd': t += to_decimal ('0', 2, dt.tm_mday); break;
            case 'e': t += to_decimal (' ', 2, dt.tm_mday); break;
            case 'm': t += to_decimal ('0', 2, dt.tm_mon + 1); break;
            case 'z': t += zone; break;
            case 'F': t += time_to_string ("%Y-%m-%d", epoch); break;
            case 'H': t += to_decimal ('0', 2, dt.tm_hour); break;
            case 'M': t += to_decimal ('0', 2, dt.tm_min); break;
            case 'R': t += time_to_string ("%H:%M", epoch); break;
            case 'S': t += to_decimal ('0', 2, dt.tm_sec); break;
            case 'T': t += time_to_string ("%H:%M:%S", epoch); break;
            case 'Y': t += to_decimal ('0', 4, dt.tm_year + 1900); break;
            }
        }
    }
    return t;
}

}//namespace http
