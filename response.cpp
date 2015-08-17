#include <map>
#include <string>
#include "server.hpp"

std::string
response_type::statuscode () const
{
    static std::map<int, std::string> status {
        {100, "Continue"},
        {101, "Switching Protocols"},
        {200, "OK"},
        {201, "Created"},
        {202, "Accepted"},
        {203, "Non-Authoritative Information"},
        {204, "No Content"},
        {205, "Reset Content"},
        {206, "Partial Content"},
        {208, "Already Reported"},
        {300, "Multiple Choices"},
        {301, "Moved Permanently"},
        {302, "Found"},
        {303, "See Other"},
        {304, "Not Modified"},
        {305, "Use Proxy"},
        {307, "Temporary Redirect"},
        {400, "Bad Request"},
        {401, "Unauthorized"},
        {402, "Payment Required"},
        {403, "Forbidden"},
        {404, "Not Found"},
        {405, "Method Not Allowed"},
        {406, "Not Acceptable"},
        {407, "Proxy Authentication Required"},
        {408, "Request Timeout"},
        {409, "Conflict"},
        {410, "Gone"},
        {411, "Length Required"},
        {412, "Precondition Failed"},
        {413, "Request Entity Too Large"},
        {414, "Request-URI Too Large"},
        {415, "Unsupported Media Type"},
        {416, "Request Range Not Satisfiable"},
        {417, "Expectation Failed"},
        {426, "Upgrade Required"},
        {428, "Precondition Required"},
        {429, "Too Many Requests"},
        {431, "Request Header Fields Too Large"},
        {500, "Internal Server Error"},
        {501, "Not Implemented"},
        {502, "Bad Gateway"},
        {503, "Service Unavailable"},
        {504, "Gateway Timeout"},
        {505, "HTTP Version Not Supported"},
        {506, "Variant Also Negotiates"},
        {511, "Network Authentication Required"},
    };
    if (status.count (code) == 0)
        return "500 Internal Server Error";
    return std::to_string (code) + " " + status[code];
}

std::string
response_type::to_string ()
{
    if (header.count ("date") == 0)
        header["date"] = to_string_time ("%a, %d %b %Y %H:%M:%S GMT");
    if (header.count ("server") == 0)
        header["server"] = "Http-Server/0.1 (Linux)";
    std::string t = http_version + " " + statuscode () + "\r\n";
    for (auto i : header) {
        std::string const& k = i.first;
        std::string const& v = i.second;
        if (k == "te")
            t += "TE";
        else if (k == "etag")
            t += "ETag";
        else if (k == "www-authenticate")
            t += "WWW-Authenticate";
        else if (k == "content-md5")
            t += "Content-MD5";
        else {
            bool ucfirst = true;
            for (int c : k) {
                t.push_back (ucfirst ? std::toupper (c) : std::tolower (c));
                ucfirst = '-' == c;
            }
        }
        t += ": " + v + "\r\n";
    }
    t += "\r\n";
    return t;
}

void
response_type::clear ()
{
    http_version = "HTTP/1.1";
    code = 500;
    content_length = -1;
    header.clear ();
    chunked = false;
    body_fd = -1;
    body.clear ();
}
