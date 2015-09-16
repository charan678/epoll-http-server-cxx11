#ifndef HTTP_HPP
#define HTTP_HPP

#include <string>
#include <vector>
#include <map>
#include <ctime>

namespace http {

std::string time_to_string (std::string const& fmt);
std::string time_to_string (std::string const& fmt, std::time_t epoch);
std::time_t time_decode (std::string const& fmt, std::string const& datime);

struct simple_token_type {
    std::string token;
    void clear () { token.clear (); }
    bool equal_token (std::string const& name) const { return token == name; }
    bool operator == (simple_token_type const& x) const { return token == x.token; }
};

struct token_type {
    std::string token;
    std::vector<std::string> parameter;
    void clear () { token.clear (); parameter.clear (); }
    bool equal_token (std::string const& name) const { return token == name; }
    bool operator == (token_type const& x) const
    {
        return token == x.token && parameter == x.parameter;
    }
};

struct content_length_type {
    int status;
    ssize_t length;
    bool operator == (content_length_type const& x) const
    {
        return status == x.status && length == x.length;
    }
};

struct etag_type {
    bool weak;
    std::string opaque;
    etag_type () : weak (false), opaque () {}
    etag_type (bool const w, std::string const& o) { weak = w; opaque = o; }

    void clear () { weak = false; opaque.clear (); }
    bool empty () const { return opaque.empty (); }

    bool equal_weak (etag_type const& x)
    {
        return opaque == x.opaque;
    }

    bool equal_strong (etag_type const& x)
    {
        return ! weak && ! x.weak && opaque == x.opaque;
    }

    bool operator == (etag_type const& x) const
    {
        return weak == x.weak && opaque == x.opaque;
    }
};

class condition_type {
public:
    enum {FAILED, OK};
    condition_type (etag_type const& et, std::time_t tm)
        : etag (et), mtime (tm) {}
    int check (std::string const& method, std::map<std::string, std::string>& header);

private:
    etag_type etag;
    std::time_t mtime;

    int if_match (std::string const& field);
    int if_none_match (std::string const& field);
    int if_unmodified_since (std::string const& field);
    int if_modified_since (std::string const& field);
};

struct request_type {
    std::string method;
    std::string uri;
    std::string http_version;
    std::map<std::string, std::string> header;
    ssize_t content_length;
    std::string body;
    void clear ();
};

struct response_type {
    int code;
    std::string http_version;
    std::map<std::string, std::string> header;
    ssize_t content_length;
    std::string body;
    int body_fd;
    bool has_body;
    bool chunked;
    ssize_t chunk_size;
    std::string statuscode () const;
    std::string to_string ();
    void clear ();
};

template<class T>
std::size_t index (std::vector<T>& fields, std::string const& name)
{
    for (std::size_t i = 0; i < fields.size (); ++i)
        if (fields[i].equal_token (name))
            return i;
    return fields.size ();
}

// 1:'0', 2:[1-9], 3:[A-Fa-f], 4: [G-Zg-z!#$%&'*+\-.^_`|~], 5:ANY
// 6:'\\', 7:'"', 8:'=', 9:';', 10:',', 11:':', 12:HTAB, 13:SP, 14:CR, 15:LF
int tocclass (int c);

bool decode (std::vector<simple_token_type>& fields, std::string const& src, int const lowerlimit);
bool decode (std::vector<token_type>& fields, std::string const& src, int const lowerlimit);
bool decode (content_length_type& field, std::string const& src);
bool decode (std::vector<etag_type>& fields, std::string const& src);

class decoder_request_type {
public:
    decoder_request_type ();
    void set_limit_nfield (std::size_t const x) { limit_nfield = x; }
    void set_limit_nbyte (std::size_t const x) { limit_nbyte = x; }
    bool put (int c, request_type& req);
    void clear ();
    bool good () const;
    bool bad () const;
    bool partial () const;

private:
    int next_state;
    std::string name;
    std::string value;
    std::string spaces;
    std::size_t nfield;
    std::size_t nbyte;
    std::size_t limit_nfield;
    std::size_t limit_nbyte;
};

class decoder_chunk_type {
public:
    enum {CHUNK_SIZE_LIMIT = 1024*1024};
    int chunk_size_limit;
    ssize_t content_length;
    int chunk_size;
    decoder_chunk_type ();
    bool put (int c, std::string& body);
    void clear ();
    bool good () const;
    bool bad () const;
    bool partial () const;

private:
    int next_state;
};

class logger_type {
public:
    static logger_type& getinstance ();
    void put_error (std::string const& s);
    void put_error (std::string const& ho, std::string const& s);
    void put_info (std::string const& s);
    void put (std::string const& ho, request_type& req, response_type& res);

private:
    logger_type ();
    ~logger_type ();
    logger_type (logger_type const&);
    logger_type& operator= (logger_type const&);
};

}//namespace http

#endif
