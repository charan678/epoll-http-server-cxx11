#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <ctime>
#include <cerrno>
#include <unistd.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include "server.hpp"

namespace http {

int
connection_type::on_accept (tcpserver_type& loop)
{
    logger_type& log = logger_type::getinstance ();
    std::string addr;
    int sock = loop.accept_client (addr);
    if (sock < 0)
        return FREE;
    else if (loop.fd_set_nonblock (sock) < 0)
        log.put_error ("fd_set_nonblock (conn_sock)");
    else if ((handle_id = loop.mplex.add (READ_EVENT|EDGE_EVENT, sock, id)) < 0)
        ;
    else {
        remote_addr = addr;
        std::time_t uptime = loop.looptime () + loop.timeout ();
        loop.mplex.mod_timer (uptime, handle_id);
        clear ();
        decoder_request_line.set_limit_nbyte (LIMIT_REQUEST_FIELD_SIZE);
        decoder_request_header.set_limit_nfield (LIMIT_REQUEST_FIELDS);
        decoder_request_header.set_limit_nbyte (LIMIT_REQUEST_FIELD_SIZE);
        return loop.register_handler (id);
    }
    if (handle_id >= 0)
        loop.mplex.del (handle_id);
    if (sock >= 0)
        close (sock);
    handle_id = -1;
    return FREE;
}

int
connection_type::on_read (tcpserver_type& loop)
{
    logger_type& log = logger_type::getinstance ();
    ssize_t n = iotransfer (loop);
    if (n > 0) {
        std::time_t uptime = loop.looptime () + loop.timeout ();
        loop.mplex.mod_timer (uptime, handle_id);
        if (iowait_mask & WRITE_EVENT) {
            if (loop.mplex.mod (WRITE_EVENT|EDGE_EVENT, handle_id) < 0)
                return loop.remove_handler (id);
        }
        return WAIT;
    }
    else if (n < 0 && EINTR == errno)
        return WAIT;
    else if (n < 0 && (EAGAIN == errno || EWOULDBLOCK == errno)) {
        loop.mplex.drop (READ_EVENT, handle_id);
        return WAIT;
    }
    else if (n < 0)
        log.put_error ("read");
    return loop.remove_handler (id);
}

int
connection_type::on_write (tcpserver_type& loop)
{
    logger_type& log = logger_type::getinstance ();
    ssize_t n = iotransfer (loop);
    if (n > 0) {
        std::time_t uptime = loop.looptime () + loop.timeout ();
        loop.mplex.mod_timer (uptime, handle_id);
        if (iowait_mask & READ_EVENT) {
            if (loop.mplex.mod (READ_EVENT|EDGE_EVENT, handle_id) < 0)
                return loop.remove_handler (id);
        }
        return WAIT;
    }
    else if (n < 0 && EINTR == errno)
        return WAIT;
    else if (n < 0 && (EAGAIN == errno || EWOULDBLOCK == errno)) {
        loop.mplex.drop (WRITE_EVENT, handle_id);
        return WAIT;
    }
    else if (n < 0 && EPIPE != errno)
        log.put_error ("write");
    return loop.remove_handler (id);
}

int
connection_type::on_timer (tcpserver_type& loop)
{
    return loop.remove_handler (id);
}

void
connection_type::on_close (tcpserver_type& loop)
{
    kont = nullptr;
    int fd = loop.mplex.fd (handle_id);
    loop.mplex.del (handle_id);
    if (fd >= 0)
        close (fd);
}

void
connection_type::clear ()
{
    keepalive_requests = 0;
    rdpos = 0;
    rdsize = 0;
    request.clear ();
    response.clear ();
    decoder_request_line.clear ();
    decoder_request_header.clear ();
    decoder_chunk.clear ();
    ioresult = 0;
    iocontinue (READ_EVENT, &connection_type::kont_request_line_read);
}

static void
to_xdigits_loop (ssize_t const n, std::string& t)
{
    if (n > 0) {
        to_xdigits_loop (n >> 4, t);
        int const x = n & 15;
        t.push_back (x < 10 ? x + '0' : x + 'a' - 10);
    }
}

static std::string
to_xdigits (ssize_t const n)
{
    if (0 == n)
        return "0";
    std::string t;
    to_xdigits_loop (n, t);
    return t;
}

static inline void
setsockopt_cork (int const sock, bool const flag)
{
    int const e = errno;
    int state = flag ? 1 : 0;
    setsockopt (sock, IPPROTO_TCP, TCP_CORK, &state, sizeof (state));
    errno = e;
}

void
connection_type::ioready ()
{
    kont_ready = true;
}

void
connection_type::iocontinue (uint32_t const mask, kont_type const kontinuation)
{
    iowait_mask = mask;
    kont = kontinuation;
    kont_ready = false;
}

void
connection_type::iocontinue (kont_type const kontinuation)
{
    kont = kontinuation;
}

void
connection_type::iostop ()
{
    kont_ready = false;
}

static inline int
ord (char c)
{
    return static_cast<uint8_t> (c);
}

ssize_t
connection_type::iotransfer (tcpserver_type& loop)
{
    ioready ();
    while (kont_ready && kont)
        (this->*kont) (loop);
    return ioresult;
}

void
connection_type::kont_request_line (tcpserver_type& loop)
{
    while (rdpos < rdsize)
        if (! decoder_request_line.put (ord (rdbuf[rdpos++]), request))
            break;
    if (decoder_request_line.partial ())
        iocontinue (READ_EVENT, &connection_type::kont_request_line_read);
    else
        iocontinue (&connection_type::kont_request_header);
}

void
connection_type::kont_request_header (tcpserver_type& loop)
{
    while (rdpos < rdsize)
        if (! decoder_request_header.put (ord (rdbuf[rdpos++]), request))
            break;
    if (decoder_request_header.partial ())
        iocontinue (READ_EVENT, &connection_type::kont_request_header_read);
    else
        prepare_request_body ();
}

void
connection_type::prepare_request_body ()
{
    if (decoder_request_line.bad () || decoder_request_header.bad ()
            || (request.http_version >= "HTTP/1.1"
                && request.header.count ("host") == 0)) {
        handler_type h;
        h.bad_request (*this);
        iocontinue (&connection_type::kont_response);
    }
    else {
        response.http_version = request.http_version;
        if (request.header.count ("transfer-encoding") > 0)
            prepare_request_chunked ();
        else if (request.header.count ("content-length") > 0)
            prepare_request_length ();
        else
            iocontinue (&connection_type::kont_dispatch);
    }
}

void
connection_type::prepare_request_chunked ()
{
    handler_type h;
    std::vector<simple_token_type> te;
    if (! decode (te, request.header["transfer-encoding"], 1)) {
        h.bad_request (*this);
        iocontinue (&connection_type::kont_response);
    }
    else if (te.size () != 1 || ! te.back ().equal_token ("chunked")) {
        h.unsupported_media_type (*this);
        iocontinue (&connection_type::kont_response);
    }
    else {
        iocontinue (&connection_type::kont_request_chunked);
    }
}

void
connection_type::kont_request_chunked (tcpserver_type& loop)
{
    while (rdpos < rdsize)
        if (! decoder_chunk.put (ord (rdbuf[rdpos++]), request.body))
            break;
    if (decoder_chunk.partial ())
        iocontinue (READ_EVENT, &connection_type::kont_request_chunked_read);
    else
        finalize_request_chunked ();
}

void
connection_type::finalize_request_chunked ()
{
    if (decoder_chunk.bad ()) {
        handler_type h;
        h.bad_request (*this);
        iocontinue (&connection_type::kont_response);
    }
    else {
        ssize_t const n = decoder_chunk.content_length;
        request.content_length = n;
        request.header["content-length"] = std::to_string (n);
        iocontinue (&connection_type::kont_dispatch);
    }
}

void
connection_type::prepare_request_length ()
{
    content_length_type canonlength;
    decode (canonlength, request.header["content-length"]);
    if (400 == canonlength.status) {
        handler_type h;
        h.bad_request (*this);
        iocontinue (&connection_type::kont_response);
    }
    else if (413 == canonlength.status) {
        handler_type h;
        h.request_entity_too_large (*this);
        iocontinue (&connection_type::kont_response);
    }
    else if (request.method != "POST" && request.method != "PUT") {
        handler_type h;
        h.request_entity_too_large (*this);
        iocontinue (&connection_type::kont_response);
    }
    else {
        request.header["content-length"] = std::to_string (canonlength.length);
        request.content_length = canonlength.length;
        iocontinue (&connection_type::kont_request_length);
    }
}

void
connection_type::kont_request_length (tcpserver_type& loop)
{
    ssize_t n = std::min (rdsize - rdpos, request.content_length);
    request.body.append (rdbuf, rdpos, n);
    rdpos += n;
    if (request.body.size () < request.content_length)
        iocontinue (READ_EVENT, &connection_type::kont_request_length_read);
    else
        iocontinue (&connection_type::kont_dispatch);
}

void
connection_type::kont_request_line_read (tcpserver_type& loop)
{
    read_with_kontinuation (loop, &connection_type::kont_request_line);
}

void
connection_type::kont_request_header_read (tcpserver_type& loop)
{
    read_with_kontinuation (loop, &connection_type::kont_request_header);
}

void
connection_type::kont_request_chunked_read (tcpserver_type& loop)
{
    read_with_kontinuation (loop, &connection_type::kont_request_chunked);
}

void
connection_type::kont_request_length_read (tcpserver_type& loop)
{
    read_with_kontinuation (loop, &connection_type::kont_request_length);
}

void
connection_type::read_with_kontinuation (tcpserver_type& loop, kont_type kontinuation)
{
    int sock = loop.mplex.fd (handle_id);
    ioresult = read (sock, &rdbuf[0], BUFFER_SIZE);
    if (ioresult <= 0)
        return iostop ();
    rdpos = 0;
    rdsize = ioresult;
    iocontinue (kontinuation);
}

void
connection_type::kont_dispatch (tcpserver_type& loop)
{
    if (request.uri == "/test") {
        handler_test_type h;
        h.process (*this);
        iocontinue (&connection_type::kont_response);
    }
    else {
        handler_file_type h;
        h.process (*this);
        iocontinue (&connection_type::kont_response);
    }
}

void
connection_type::kont_response (tcpserver_type& loop)
{
    prepare_response ();
    iocontinue (WRITE_EVENT, &connection_type::kont_response_header);
}

void
connection_type::prepare_response ()
{
    decide_transfer_encoding ();
    response.has_body = true;
    int code = response.code;
    if (request.method == "HEAD" || 304 == code)
        response.has_body = false;
    else if ((100 <= code && code < 200) || 204 == code) {
        response.has_body = false;
        if (response.header.count ("transfer-encoding") > 0)
            response.header.erase ("transfer-encoding");
        if (response.header.count ("content-length") > 0)
            response.header.erase ("content-length");
    }
    wrbuf = response.to_string ();
    wrpos = 0;
    wrsize = wrbuf.size ();
}

void
connection_type::decide_transfer_encoding ()
{
    if (response.header.count ("transfer-encoding") > 0) {
        std::vector<simple_token_type> te;
        if (! decode (te, response.header["transfer-encoding"], 1)
                || te.size () != 1 || ! te.back ().equal_token ("chunked"))
            response.header.erase ("transfer-encoding");
    }
    if (response.header.count ("content-length") > 0) {
        content_length_type canonlength;
        decode (canonlength, response.header.at ("content-length"));
        if (canonlength.status != 200)
            response.header.erase ("content-length");
        else
            response.header["content-length"] = std::to_string (canonlength.length);
    }
    response.chunked = false;
    if (response.http_version < "HTTP/1.1") {
        if (response.header.count ("transfer-encoding") > 0)
            response.header.erase ("transfer-encoding");
    }
    else if (response.header.count ("transfer-encoding") > 0) {
        response.chunked = true;
        if (response.header.count ("content-length") > 0)
            response.header.erase ("content-length");
    }
    if (response.header.count ("transfer-encoding") == 0
            && response.header.count ("content-length") == 0)
        response.header["connection"] = "close";
}

void
connection_type::kont_response_header (tcpserver_type& loop)
{
    int sock = loop.mplex.fd (handle_id);
    ioresult = write (sock, &wrbuf[wrpos], wrsize - wrpos);
    if (ioresult <= 0)
        return iostop ();
    wrpos += ioresult;
    if (wrpos < wrsize)
        iocontinue (WRITE_EVENT, &connection_type::kont_response_header);
    else if (! response.has_body)
        iocontinue (&connection_type::kont_response_end);
    else
        prepare_response_body ();
}

void
connection_type::prepare_response_body ()
{
    wrpos = 0;
    if (response.chunked) {
        if (response.body_fd < 0)
            response.content_length = response.body.size ();
        response.chunk_size = std::min (
            response.content_length - wrpos, BUFFER_SIZE - 16);
        wrbuf = to_xdigits (response.chunk_size) + "\r\n";
        wrpos1 = 0;
        iocontinue (WRITE_EVENT, &connection_type::kont_response_chunk_header);
    }
    else if (response.body_fd >= 0) {
        iocontinue (WRITE_EVENT, &connection_type::kont_response_file_length);
    }
    else {
        wrsize = response.body.size ();
        iocontinue (WRITE_EVENT, &connection_type::kont_response_body_length);
    }
}

void
connection_type::kont_response_chunk_header (tcpserver_type& loop)
{
    int sock = loop.mplex.fd (handle_id);
    setsockopt_cork (sock, true);
    ioresult = write (sock, &wrbuf[wrpos1], wrbuf.size () - wrpos1);
    if (ioresult <= 0)
        return iostop ();
    wrpos1 += ioresult;
    if (wrpos1 < wrbuf.size ())
        iocontinue (WRITE_EVENT, &connection_type::kont_response_chunk_header);
    else if (response.chunk_size > 0) {
        wrsize = wrpos + response.chunk_size;
        iocontinue (WRITE_EVENT, &connection_type::kont_response_chunk_body);
    }
    else {
        wrbuf = "\r\n";
        wrpos1 = 0;
        iocontinue (WRITE_EVENT, &connection_type::kont_response_chunk_crlf);
    }
}

void
connection_type::kont_response_chunk_body (tcpserver_type& loop)
{
    int sock = loop.mplex.fd (handle_id);
    if (response.body_fd < 0)
        ioresult = write (sock, &response.body[wrpos], wrsize - wrpos);
    else
        ioresult = sendfile (sock, response.body_fd, nullptr, wrsize - wrpos);
    if (ioresult <= 0)
        return iostop ();
    wrpos += ioresult;
    if (wrpos < wrsize)
        iocontinue (WRITE_EVENT, &connection_type::kont_response_chunk_body);
    else {
        wrbuf = "\r\n";
        wrpos1 = 0;
        iocontinue (WRITE_EVENT, &connection_type::kont_response_chunk_crlf);
    }
}

void
connection_type::kont_response_chunk_crlf (tcpserver_type& loop)
{
    int sock = loop.mplex.fd (handle_id);
    ioresult = write (sock, &wrbuf[wrpos1], wrbuf.size () - wrpos1);
    if (ioresult <= 0)
        return iostop ();
    wrpos1 += ioresult;
    if (wrpos1 < wrbuf.size ())
        iocontinue (WRITE_EVENT, &connection_type::kont_response_chunk_crlf);
    else if (response.chunk_size == 0) {
        setsockopt_cork (sock, false);
        iocontinue (&connection_type::kont_response_end);
    }
    else {
        setsockopt_cork (sock, false);
        response.chunk_size = std::min (
            response.content_length - wrpos, BUFFER_SIZE - 16);
        wrbuf = to_xdigits (response.chunk_size) + "\r\n";
        wrpos1 = 0;
        iocontinue (WRITE_EVENT, &connection_type::kont_response_chunk_header);
    }
}

void
connection_type::kont_response_file_length (tcpserver_type& loop)
{
    int sock = loop.mplex.fd (handle_id);
    ioresult = sendfile (sock, response.body_fd, nullptr, BUFFER_SIZE);
    if (ioresult < 0)
        iostop ();
    else if (ioresult > 0) {
        wrpos += ioresult;
        iocontinue (WRITE_EVENT, &connection_type::kont_response_file_length);
    }
    else {
        ioresult = BUFFER_SIZE;
        iocontinue (&connection_type::kont_response_end);
    }
}

void
connection_type::kont_response_body_length (tcpserver_type& loop)
{
    int sock = loop.mplex.fd (handle_id);
    ioresult = write (sock, &response.body[wrpos], wrsize - wrpos);
    if (ioresult <= 0)
        return iostop ();
    wrpos += ioresult;
    if (wrpos < wrsize)
        iocontinue (WRITE_EVENT, &connection_type::kont_response_body_length);
    else
        iocontinue (&connection_type::kont_response_end);
}

void
connection_type::kont_response_end (tcpserver_type& loop)
{
    int sock = loop.mplex.fd (handle_id);
    if (! finalize_response ())
        iocontinue (&connection_type::kont_request_line);
    else {
        shutdown (sock, SHUT_WR);
        iocontinue (READ_EVENT, &connection_type::kont_teardown);
    }
}

bool
connection_type::finalize_response ()
{
    if (response.body_fd >= 0) {
        close (response.body_fd);
        response.body_fd = -1;
    }
    response.content_length = wrpos;
    logger_type& log = logger_type::getinstance ();
    log.put (remote_addr, request, response);
    bool teardown = done_connection ();
    response.clear ();
    request.clear ();
    decoder_request_line.clear ();
    decoder_request_header.clear ();
    decoder_chunk.clear ();
    return teardown;
}

bool
connection_type::done_connection ()
{
    std::vector<simple_token_type> rqconn;
    std::vector<simple_token_type> rsconn;
    if (MAX_KEEPALIVE_REQUESTS <= ++keepalive_requests)
        return true;
    if (request.header.count ("connection") > 0)
        decode (rqconn, request.header["connection"], 1);
    if (response.header.count ("connection") > 0)
        decode (rsconn, response.header["connection"], 1);
    if (index (rqconn, "close") < rqconn.size ())
        return true;
    if (index (rsconn, "close") < rsconn.size ())
        return true;
    if (response.http_version < "HTTP/1.1")
        return index (rqconn, "keep-alive") == rqconn.size ();
    return false;
}

void
connection_type::kont_teardown (tcpserver_type& loop)
{
    int sock = loop.mplex.fd (handle_id);
    ioresult = read (sock, &rdbuf[0], BUFFER_SIZE);
    if (ioresult > 0)
        iocontinue (READ_EVENT, &connection_type::kont_teardown);
    else
        iostop ();
}

}//namespace http
