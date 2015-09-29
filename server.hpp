#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <vector>
#include <map>
#include "http.hpp"
#include "html-builder.hpp"

namespace http {

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

class mplex_io_type {
public:
    mplex_io_type (std::size_t const n);
    virtual ~mplex_io_type () {};
    virtual int add (uint32_t const trigger, int const fd, std::size_t const handler_id) = 0;
    virtual int mod (uint32_t const trigger, std::size_t const id) = 0;
    virtual int drop (uint32_t const events, std::size_t const id) = 0;
    virtual int del (std::size_t const id) = 0;
    virtual int add_timer (std::time_t const uptime, std::size_t const handler_id);
    virtual int mod_timer (std::time_t const uptime, std::size_t const id);
    virtual int stop_timer (std::size_t const id);
    virtual int wait (int msec) = 0;
    virtual void run_timer ();
    bool empty () { return handles.empty (READY); }
    int begin () { return handles[READY].next; }
    int fd (std::size_t const id) { return handles[id].fd; }
    int handler_id (std::size_t const id) { return handles[id].handler_id; }
    int events (std::size_t const id) { return handles[id].events & handles[id].ev_mask; }
    int next (std::size_t const id) { return handles[id].next; }
    int end () { return READY; }
    std::time_t looptime () const { return looptime_; }

protected:
    int max_events;
    std::time_t looptime_;
    ring_in_vector<handle_type> handles;
    std::multimap<std::time_t, int> timers;

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

class mplex_epoll_type : public mplex_io_type {
public:
    mplex_epoll_type (std::size_t const n);
    ~mplex_epoll_type ();
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

class connection_type {
public:
    std::size_t const id;
    std::size_t prev, next;
    ssize_t handle_id;
    int state;
    std::string remote_addr;
    response_type response;
    request_type request;

    connection_type (std::size_t a, std::size_t b, std::size_t c)
        : id (a), prev (b), next (c),
          handle_id (-1), state (FREE), remote_addr (),
          response (), request (),
          kont_ready (false), kont (1), rdbuf (BUFFER_SIZE, '\0'), wrbuf (),
          rdpos (0), rdsize (0), wrpos (0), wrpos1 (0), wrsize (0),
          decoder_request_line (), decoder_request_header (),
          decoder_chunk () {}
    ssize_t iotransfer (tcpserver_type& loop);
    int on_accept (tcpserver_type& loop);
    int on_read (tcpserver_type& loop);
    int on_write (tcpserver_type& loop);
    int on_timer (tcpserver_type& loop);
    void on_close (tcpserver_type& loop);
    void clear ();

private:
    enum {
        KREQUEST_LINE,
        KREQUEST_HEADER,
        KREQUEST_CHUNKED,
        KREQUEST_LENGTH,
        KREQUEST_LINE_READ,
        KREQUEST_HEADER_READ,
        KREQUEST_CHUNKED_READ,
        KREQUEST_LENGTH_READ,
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
    bool kont_ready;
    int kont;
    std::string rdbuf;
    std::string wrbuf;
    ssize_t rdpos;
    ssize_t rdsize;
    ssize_t wrpos;
    ssize_t wrpos1;
    ssize_t wrsize;
    decoder_request_line_type decoder_request_line;
    decoder_request_header_type decoder_request_header;
    decoder_chunk_type decoder_chunk;
    uint32_t iowait_mask;
    ssize_t ioresult;
    int keepalive_requests;

    void kont_request_line (tcpserver_type& loop);
    void kont_request_header (tcpserver_type& loop);
    void kont_request_chunked (tcpserver_type& loop);
    void kont_request_length (tcpserver_type& loop);
    void kont_request_read (tcpserver_type& loop);
    void kont_dispatch (tcpserver_type& loop);
    void kont_response (tcpserver_type& loop);
    void kont_response_header (tcpserver_type& loop);
    void kont_response_chunk_header (tcpserver_type& loop);
    void kont_response_chunk_body (tcpserver_type& loop);
    void kont_response_chunk_crlf (tcpserver_type& loop);
    void kont_response_file_length (tcpserver_type& loop);
    void kont_response_body_length (tcpserver_type& loop);
    void kont_response_end (tcpserver_type& loop);
    void kont_teardown (tcpserver_type& loop);

    void ioready ();
    void iocontinue (uint32_t const mask, int const kontinuation);
    void iocontinue (int const kontinuation);
    void iostop ();
    void prepare_request_body ();
    void prepare_request_chunked ();
    void finalize_request_chunked ();
    void prepare_request_length ();
    void prepare_response ();
    void decide_transfer_encoding ();
    void prepare_response_body ();
    bool finalize_response ();
    bool done_connection ();
};

class handler_type {
public:
    handler_type () {}
    virtual ~handler_type () {}

    virtual bool process (connection_type& r);
    virtual bool get (connection_type& r);
    virtual bool put (connection_type& r);
    virtual bool post (connection_type& r);

    bool not_modified (connection_type& r);

    bool bad_request (connection_type& r);
    bool not_found (connection_type& r);
    bool method_not_allowed (connection_type& r);
    bool request_timeout (connection_type& r);
    bool length_required (connection_type& r);
    bool precondition_failed (connection_type& r);
    bool request_entity_too_large (connection_type& r);
    bool unsupported_media_type (connection_type& r);

    void error_start (connection_type& r, html_builder_type& html, int code);
    void error_end (connection_type& r, html_builder_type& html);
};

class handler_test_type : public handler_type {
public:
    handler_test_type () {}
    ~handler_test_type () {}

    virtual bool get (connection_type& r);
    virtual bool post (connection_type& r);
};

class handler_file_type : public handler_type {
public:
    handler_file_type () {}
    ~handler_file_type () {}

    virtual bool get (connection_type& r);

private:
    std::string mime_type (std::string const& path) const;
};

class tcpserver_type {
public:
    enum {STOP, RUN};
    mplex_io_type& mplex;
    tcpserver_type (std::size_t n, int to, mplex_io_type& m)
        :  mplex (m), max_connections (n), timeout_ (to),
          listen_port (SERVER_PORT), listen_sock (-1), handlers () {}
    void run (int const port, int const backlog);
    int register_handler (std::size_t const handler_id);
    int remove_handler (std::size_t const handler_id);
    int timeout () const { return timeout_; }
    std::time_t looptime () const { return mplex.looptime (); }
    int listen_socket_create (int const port, int const backlog);
    int accept_client (std::string& remote_addr);
    int fd_set_nonblock (int fd);

private:
    std::size_t max_connections;
    int timeout_;
    int listen_port;
    int listen_sock;
    ring_in_vector<connection_type> handlers;

    int initialize (int const port, int const backlog);
    void shutdown ();
};

}//namespace http

#endif
