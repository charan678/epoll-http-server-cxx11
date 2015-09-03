#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <vector>
#include <map>
#include <ctime>

enum {
    SERVER_PORT = 10080,
    BACKLOG = 10,
    MAX_CONNECTIONS = 10,
    LISTENER_COUNT = 1,
    TIMEOUT = 60, // seconds

    MAX_KEEPALIVE_REQUESTS = 5,
    LIMIT_REQUEST_FIELDS = 100,
    LIMIT_REQUEST_FIELD_SIZE = 8190,

    BUFFER_SIZE = 4096,

    READ_EVENT = 1,
    WRITE_EVENT = 2,
    TIMER_EVENT = 4,
    EDGE_EVENT = 0x8000000,
    FREE = 0,
    WAIT = 1,
    READY = 2,
};

static inline std::string documentroot () { return "public"; }

std::string to_string_time (std::string const& fmt);
std::string to_string_time (std::string const& fmt, std::time_t epoch);
std::time_t decode_time (std::string const& fmt, std::string const& datime);

template <class NODE_T>
class ring_in_vector {
public:
    ring_in_vector () : node () {}

    void
    resize (std::size_t n)
    {
        if (n == 0)
            n = 16;
        node.clear ();
        for (std::size_t i = 0; i < n; ++i)
            node.emplace_back (i, (n + i - 1) % n, (i + 1) % n);
    }

    bool
    empty (std::size_t i) const
    {
        return node[i].next == i;
    }

    void
    insert (std::size_t const j, std::size_t const k)
    {
        node[k].prev = node[j].prev;
        node[k].next = j;
        node[node[k].next].prev = k;
        node[node[k].prev].next = k;
    }

    void
    erase (std::size_t const i)
    {
        node[node[i].next].prev = node[i].prev;
        node[node[i].prev].next = node[i].next;
        node[i].prev = i;
        node[i].next = i;
    }

    NODE_T& operator[] (std::size_t i)
    {
        return node[i];
    }

    std::size_t size () const { return node.size (); }

private:
    std::vector<NODE_T> node;
};

class response_type {
public:
    std::string http_version;
    int code;
    ssize_t content_length;
    std::map<std::string, std::string> header;
    bool has_body;
    bool chunked;
    ssize_t chunk_size;
    int body_fd;
    std::string body;
    std::string statuscode () const;
    std::string to_string ();
    void clear ();
};

class request_type {
public:
    std::string method;
    std::string uri;
    std::string http_version;
    std::map<std::string, std::string> header;
    ssize_t content_length;
    std::string body;
    void clear ();
    bool header_token (std::string const& str, std::vector<std::string>& tokens);
    ssize_t canonical_length (std::string const& s);
};

class request_decoder_type {
public:
    request_decoder_type ();
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
};

class chunk_decoder_type {
public:
    enum {CHUNK_SIZE_LIMIT = 1024*1024};
    int chunk_size_limit;
    ssize_t content_length;
    int chunk_size;
    chunk_decoder_type ();
    bool put (int c, std::string& body);
    void clear ();
    bool good () const;
    bool bad () const;
    bool partial () const;

private:
    int next_state;
};

class html_builder_type {
public:
    html_builder_type ();
    html_builder_type& operator << (char const* s);
    html_builder_type& operator << (int const i);
    html_builder_type& operator << (std::string const& s);
    std::string const& string () const;

private:
    std::string buffer;
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

struct handle_type {
    std::size_t const id;
    std::size_t prev, next;
    bool ev_permit;
    int fd;
    std::time_t uptime;
    std::size_t handler_id;
    int state;
    uint32_t ev_mask;
    uint32_t events;
    handle_type (std::size_t a, std::size_t b, std::size_t c)
        : id (a), prev (b), next (c), ev_permit (false),
          fd (-1), uptime (0), handler_id (0),
          state (FREE), ev_mask (0), events (0) {} 
};

class io_mplex_type {
public:
    io_mplex_type (std::size_t const n);
    virtual ~io_mplex_type () {};
    virtual int add (uint32_t const trigger, int const fd, std::size_t const handler_id) = 0;
    virtual int mod (uint32_t const trigger, std::size_t const id) = 0;
    virtual int drop (uint32_t const events, std::size_t const id) = 0;
    virtual int del (std::size_t const id) = 0;
    virtual int add_timer (std::time_t const uptime, std::size_t const handler_id);
    virtual int mod_timer (std::time_t const uptime, std::size_t const id);
    virtual int stop_timer (std::size_t const id);
    virtual int wait (int msec) = 0;
    bool empty () { return handles.empty (READY); }
    int begin () { return handles[READY].next; }
    int fd (std::size_t const id) { return handles[id].fd; }
    int handler_id (std::size_t const id) { return handles[id].handler_id; }
    int events (std::size_t const id) { return handles[id].events & handles[id].ev_mask; }
    int next (std::size_t const id) { return handles[id].next; }
    int end () { return READY; }

protected:
    int max_events;
    ring_in_vector<handle_type> handles;
    std::multimap<std::time_t, int> timers;

    virtual void run_timer ();

    inline int range_check (std::size_t const id)
    {
        if (id <= READY || id >= handles.size ()) {
            errno = ENOENT;
            return -1;
        }
        if (FREE == handles[id].state) {
            errno = ENOENT;
            return -1;
        }
        return id;
    }
};

class epoll_mplex_type : public io_mplex_type {
public:
    epoll_mplex_type (std::size_t const n);
    ~epoll_mplex_type ();
    int add (uint32_t const trigger, int const fd, std::size_t const handler_id);
    int mod (uint32_t const trigger, std::size_t const id);
    int drop (uint32_t const events, std::size_t const id);
    int del (std::size_t const id);
    int wait (int msec);

private:
    int epoll_fd;
    struct epoll_event* evset;
};

class tcpserver_type;

class event_handler_type {
public:
    enum {
        KREQUEST_HEADER_READ,
        KREQUEST_HEADER,
        KREQUEST_CHUNKED_READ,
        KREQUEST_CHUNKED,
        KREQUEST_LENGTH_READ,
        KREQUEST_LENGTH,
        KDISPATCH,
        KRESPONSE,
        KRESPONSE_HEADER,
        KRESPONSE_CHUNK_HEADER,
        KRESPONSE_CHUNK_BODY,
        KRESPONSE_CHUNK_CRLF,
        KRESPONSE_FILE_LENGTH,
        KRESPONSE_BODY_LENGTH,
        KRESPONSE_END,
        KTEARDOWN,
    };
    std::size_t const id;
    std::size_t prev, next;
    ssize_t handle_id;
    int state;
    std::string remote_addr;
    int kont;
    std::string rdbuf;
    std::string wrbuf;
    ssize_t rdpos;
    ssize_t rdsize;
    ssize_t wrpos;
    ssize_t wrpos1;
    ssize_t wrsize;
    response_type response;
    request_type request;
    request_decoder_type reqdecoder;
    chunk_decoder_type chunkdecoder;
    uint32_t iowait_mask;
    ssize_t ioresult;
    int keepalive_requests;

    event_handler_type (std::size_t a, std::size_t b, std::size_t c)
        : id (a), prev (b), next (c),
          handle_id (-1), state (FREE), remote_addr (),
          kont (1), rdbuf (BUFFER_SIZE, '\0'), wrbuf (),
          rdpos (0), rdsize (0), wrpos (0), wrpos1 (0), wrsize (0),
          response (), request (), reqdecoder (), chunkdecoder () {}
    ssize_t iotransfer (tcpserver_type& loop);
    ssize_t iocontinue (uint32_t const mask, int const kontinuation);
    int on_accept (tcpserver_type& loop);
    int on_read (tcpserver_type& loop);
    int on_write (tcpserver_type& loop);
    int on_timer (tcpserver_type& loop);
    void on_close (tcpserver_type& loop);
    void clear ();

    void prepare_request_body ();
    void prepare_request_chunked ();
    void finalize_request_chunked ();
    void prepare_request_length ();
    void prepare_response ();
    void decide_transfer_encoding ();
    void guess_close ();
    bool prepare_response_body ();
    bool finalize_response ();
    bool done_connection ();
};

class message_handler_type {
public:
    message_handler_type () {}
    virtual ~message_handler_type () {}

    virtual bool process (event_handler_type& r);
    virtual bool get (event_handler_type& r);
    virtual bool put (event_handler_type& r);
    virtual bool post (event_handler_type& r);

    bool not_modified (event_handler_type& r);

    bool bad_request (event_handler_type& r);
    bool not_found (event_handler_type& r);
    bool method_not_allowed (event_handler_type& r);
    bool request_timeout (event_handler_type& r);
    bool length_required (event_handler_type& r);
    bool precondition_failed (event_handler_type& r);
    bool request_entity_too_large (event_handler_type& r);
    bool unsupported_media_type (event_handler_type& r);

    void error_start (event_handler_type& r, html_builder_type& html, int code);
    void error_end (event_handler_type& r, html_builder_type& html);
};

class test_handler_type : public message_handler_type {
public:
    test_handler_type () {}
    ~test_handler_type () {}

    virtual bool get (event_handler_type& r);
    virtual bool post (event_handler_type& r);
};

class file_handler_type : public message_handler_type {
public:
    file_handler_type () {}
    ~file_handler_type () {}

    virtual bool get (event_handler_type& r);

private:
    std::string mime_type (std::string const& path) const;
};

class request_condition_type {
public:
    enum {BAD, FAILED, OK};
    request_condition_type (std::string const& et, std::time_t tm)
        : etag (et), mtime (tm) {}
    int check (std::string const& method, std::map<std::string, std::string>& header);

private:
    std::string etag;
    std::time_t mtime;

    int if_match (std::string const& field);
    int if_none_match (std::string const& field);
    int if_unmodified_since (std::string const& field);
    int if_modified_since (std::string const& field);
    bool strong_compare (std::string const& etag0, std::string const& etag1);
    bool weak_compare (std::string const& etag0, std::string const& etag1);
    bool decode_field (std::string const& field, std::vector<std::string>& list);
};

class tcpserver_type {
public:
    enum {STOP, RUN};
    io_mplex_type& mplex;
    tcpserver_type (std::size_t n, int to, io_mplex_type& m)
        :  mplex (m), max_connections (n), timeout_ (to),
          listen_port (SERVER_PORT), listen_sock (-1), handlers () {}
    void run (int const port, int const backlog);
    int register_handler (std::size_t const handler_id);
    int remove_handler (std::size_t const handler_id);
    int timeout () const { return timeout_; }
    int listen_socket_create (int const port, int const backlog);
    int accept_client (std::string& remote_addr);
    int fd_set_nonblock (int fd);

private:
    std::size_t max_connections;
    int timeout_;
    int listen_port;
    int listen_sock;
    ring_in_vector<event_handler_type> handlers;

    int initialize (int const port, int const backlog);
    void shutdown ();
};

#endif
