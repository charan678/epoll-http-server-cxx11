#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "server.hpp"

namespace http {

struct location_type {
    std::vector<std::string> dirs;
    std::string name;
    std::string ext;
    location_type () : dirs (), name (), ext (), ok_ (false) {}
    bool splitpath (std::string const& path);
    bool ok () const { return ok_; }
    std::string catpath () const;

private:
    bool ok_;
};

bool
handler_file_type::get (http::connection_type& r)
{
    static const std::string httpdate ("%a, %d %b %Y %H:%M:%S GMT");
    location_type loc;
    if (! loc.splitpath (r.request.uri))
        return not_found (r);
    std::string path = documentroot () + loc.catpath ();
    r.response.code = 200;
    struct stat st;
    if (stat (path.c_str (), &st) < 0 || ! S_ISREG (st.st_mode))
        return not_found (r);
    std::string etag = "\"" + std::to_string (st.st_ino) 
                      + "-" + std::to_string (st.st_mtime)
                      + "-" + std::to_string (st.st_size) + "\"";
    condition_type precond ({false, etag}, st.st_mtime);
    int code = precond.check (r.request.method, r.request.header);
    if (400 == code)
        return bad_request (r);
    if (412 == code)
        return precondition_failed (r);
    r.response.content_length = st.st_size;
    r.response.header["content-length"] = std::to_string (st.st_size);
    r.response.header["content-type"] = mime_type (loc.ext);
    r.response.header["etag"] = etag;
    r.response.header["last-modified"] = time_to_string (httpdate, st.st_mtime);
    if (304 == code)
        return not_modified (r);
    if (r.request.method == "HEAD")
        return true;
    int file_fd = open (path.c_str (), O_RDONLY);
    if (file_fd < 0)
        return not_found (r);
    r.response.body_fd = file_fd;
    return true;
}

std::string
handler_file_type::mime_type (std::string const& ext) const
{
    static const std::vector<std::string> mime {
        "html",  "text/html; charset=UTF-8",
        "css",   "text/css",
        "md",    "text/markdown",
        "jpeg",  "image/jpeg",
        "png",   "image/png",
        "gif",   "image/gif",
        "js",    "application/javascript",
        "json",  "application/json",
        "pdf",   "application/pdf",
        "xml",   "application/xml",
        "xhtml", "application/xhtml+xml",
        "atom",  "application/atom+xml",
        "ico",   "image/vnd.microsoft.icon",
        "txt",   "text/plain; charset=UTF-8",
    };
    for (int i = 0; i < mime.size (); i += 2)
        if (mime[i] == ext)
            return mime[i + 1];
    return mime.back ();
}

bool
location_type::splitpath (std::string const& path)
{
    static const int SHIFT[8][5] = {
        //  /   .  -_  [[:alnum:]]
        {0, 0,  0,  0,  0},
        {0, 2,  0,  0,  0}, // S1: '/' S2
        {1, 0,  0,  0,  3}, // S2: [[:alnum:]] S3
        {3, 2,  5,  4,  3}, // S3: [[:alnum:]] S3 | '/' S2 | [-_] S4 | [.] S5
        {2, 0,  0,  0,  3}, // S4: [[:alnum:]] S3
        {2, 0,  0,  0,  6}, // S5: [[:alnum:]] S6
        {7, 2,  5,  7,  6}, // S6: [[:alnum:]] S6 | '/' S2 | [-_] S7 | [.] S5
        {6, 0,  0,  0,  6}, // S7: [[:alnum:]] S6
    };
    dirs.clear ();
    name.clear ();
    ext.clear ();
    int next_state = 1;
    for (int c : path) {
        int cls = '/' == c ? 1 : '.' == c ? 2 : '-' == c ? 3 : '_' == c ? 3
                : std::isalnum (c) ? 4 : 0;
        int prev_state = next_state;
        if (0 == cls || 0 == SHIFT[prev_state][cls])
            return false;
        next_state = SHIFT[prev_state][cls];
        if (2 == next_state) {
            dirs.push_back (name);
            name.clear ();
            ext.clear ();
        }
        if (5 == next_state)
            ext.clear ();
        if (2 & SHIFT[next_state][0])
            name.push_back (c);
        if (4 & SHIFT[next_state][0])
            ext.push_back (std::tolower (c));
    }
    if (3 == next_state) {
        dirs.push_back (name);
        next_state = 2;
    }
    if (2 == next_state) {
        name = "index.html";
        ext = "html";
    }
    ok_ = (1 & SHIFT[next_state][0]) != 0;
    return ok_;
}

std::string
location_type::catpath () const
{
    std::string parent;
    for (auto& x : dirs)
        parent += x + "/";
    return parent + name;
}

}//namespace http
