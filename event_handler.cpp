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

int
event_handler_type::on_accept (tcpserver_type& loop)
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
        std::time_t uptime = std::time (nullptr) + loop.timeout ();
        loop.mplex.mod_timer (uptime, handle_id);
        clear ();
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
event_handler_type::on_read (tcpserver_type& loop)
{
    logger_type& log = logger_type::getinstance ();
    ssize_t n = iotransfer (loop);
    if (n > 0) {
        std::time_t uptime = std::time (nullptr) + loop.timeout ();
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
event_handler_type::on_write (tcpserver_type& loop)
{
    logger_type& log = logger_type::getinstance ();
    ssize_t n = iotransfer (loop);
    if (n > 0) {
        std::time_t uptime = std::time (nullptr) + loop.timeout ();
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
event_handler_type::on_timer (tcpserver_type& loop)
{
    return loop.remove_handler (id);
}

void
event_handler_type::on_close (tcpserver_type& loop)
{
    int fd = loop.mplex.fd (handle_id);
    loop.mplex.del (handle_id);
    if (fd >= 0)
        close (fd);
}

void
event_handler_type::clear ()
{
    keepalive_requests = 0;
    rdpos = 0;
    rdsize = 0;
    request.clear ();
    response.clear ();
    reqdecoder.clear ();
    chunkdecoder.clear ();
    ioresult = 0;
    iocontinue (READ_EVENT, KREQUEST_HEADER_READ);
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

ssize_t
event_handler_type::iocontinue (uint32_t const mask, int const kontinuation)
{
    iowait_mask = mask;
    kont = kontinuation;
    return ioresult;
}

ssize_t
event_handler_type::iotransfer (tcpserver_type& loop)
{
    for (;;) {
        if (KREQUEST_HEADER_READ == kont
                || KREQUEST_CHUNKED_READ == kont
                || KREQUEST_LENGTH_READ == kont) {
            int sock = loop.mplex.fd (handle_id);
            ioresult = read (sock, &rdbuf[0], BUFFER_SIZE);
            if (ioresult <= 0)
                break;
            rdpos = 0;
            rdsize = ioresult;
            kont = KREQUEST_HEADER_READ == kont ? KREQUEST_HEADER
                 : KREQUEST_CHUNKED_READ == kont ? KREQUEST_CHUNKED
                 : KREQUEST_LENGTH;
        }
        else if (KREQUEST_HEADER == kont) {
            while (rdpos < rdsize)
                if (! reqdecoder.put (rdbuf[rdpos++], request))
                    break;
            if (reqdecoder.partial ()) {
                return iocontinue (READ_EVENT, KREQUEST_HEADER_READ);
            }
            prepare_request_body ();
        }
        else if (KREQUEST_CHUNKED == kont) {
            while (rdpos < rdsize)
                if (! chunkdecoder.put (rdbuf[rdpos++], request.body))
                    break;
            if (chunkdecoder.partial ()) {
                return iocontinue (READ_EVENT, KREQUEST_CHUNKED_READ);
            }
            finalize_request_chunked ();
        }
        else if (KREQUEST_LENGTH == kont) {
            ssize_t n = std::min (rdsize - rdpos, request.content_length);
            request.body.append (rdbuf, rdpos, n);
            rdpos += n;
            if (request.body.size () < request.content_length) {
                return iocontinue (READ_EVENT, KREQUEST_LENGTH_READ);
            }
            kont = KDISPATCH;
        }
        else if (KDISPATCH == kont) {
            if (request.uri == "/test") {
                test_handler_type h;
                h.process (*this);
            }
            else {
                file_handler_type h;
                h.process (*this);
            }
            kont = KRESPONSE;
        }
        else if (KRESPONSE == kont) {
            prepare_response ();
            return iocontinue (WRITE_EVENT, KRESPONSE_HEADER);
        }
        else if (KRESPONSE_HEADER == kont) {
            int sock = loop.mplex.fd (handle_id);
            ioresult = write (sock, &wrbuf[wrpos], wrsize - wrpos);
            if (ioresult <= 0)
                break;
            wrpos += ioresult;
            if (wrpos < wrsize) {
                return iocontinue (WRITE_EVENT, KRESPONSE_HEADER);
            }
            if (prepare_response_body ()) {
                return iocontinue (WRITE_EVENT, kont);
            }
            kont = KRESPONSE_END;
        }
        else if (KRESPONSE_CHUNK_HEADER == kont) {
            int sock = loop.mplex.fd (handle_id);
            setsockopt_cork (sock, true);
            ioresult = write (sock, &wrbuf[wrpos1], wrbuf.size () - wrpos1);
            if (ioresult <= 0)
                break;
            wrpos1 += ioresult;
            if (wrpos1 < wrbuf.size ()) {
                return iocontinue (WRITE_EVENT, KRESPONSE_CHUNK_HEADER);
            }
            else if (response.chunk_size > 0) {
                wrsize = wrpos + response.chunk_size;
                return iocontinue (WRITE_EVENT, KRESPONSE_CHUNK_BODY);
            }
            else {
                wrbuf = "\r\n";
                wrpos1 = 0;
                return iocontinue (WRITE_EVENT, KRESPONSE_CHUNK_CRLF);
            }
        }
        else if (KRESPONSE_CHUNK_BODY == kont) {
            int sock = loop.mplex.fd (handle_id);
            ssize_t n = wrsize - wrpos;
            if (response.body_fd < 0)
                ioresult = write (sock, &response.body[wrpos], n);
            else
                ioresult = sendfile (sock, response.body_fd, nullptr, n);
            if (ioresult <= 0)
                break;
            wrpos += ioresult;
            if (wrpos < wrsize) {
                return iocontinue (WRITE_EVENT, KRESPONSE_CHUNK_BODY);
            }
            else {
                wrbuf = "\r\n";
                wrpos1 = 0;
                return iocontinue (WRITE_EVENT, KRESPONSE_CHUNK_CRLF);
            }
        }
        else if (KRESPONSE_CHUNK_CRLF == kont) {
            int sock = loop.mplex.fd (handle_id);
            ioresult = write (sock, &wrbuf[wrpos1], wrbuf.size () - wrpos1);
            if (ioresult <= 0)
                break;
            wrpos1 += ioresult;
            if (wrpos1 < wrbuf.size ()) {
                return iocontinue (WRITE_EVENT, KRESPONSE_CHUNK_CRLF);
            }
            setsockopt_cork (sock, false);
            if (response.chunk_size > 0) {
                response.chunk_size = std::min (
                    response.content_length - wrpos, BUFFER_SIZE - 16);
                wrbuf = to_xdigits (response.chunk_size) + "\r\n";
                wrpos1 = 0;
                return iocontinue (WRITE_EVENT, KRESPONSE_CHUNK_HEADER);
            }
            kont = KRESPONSE_END;
        }
        else if (KRESPONSE_FILE_LENGTH == kont) {
            int sock = loop.mplex.fd (handle_id);
            ioresult = sendfile (sock, response.body_fd, nullptr, BUFFER_SIZE);
            if (ioresult < 0)
                break;
            if (ioresult > 0) {
                wrpos += ioresult;
                return iocontinue (WRITE_EVENT, KRESPONSE_FILE_LENGTH);
            }
            ioresult = BUFFER_SIZE;
            kont = KRESPONSE_END;
        }
        else if (KRESPONSE_BODY_LENGTH == kont) {
            int sock = loop.mplex.fd (handle_id);
            ioresult = write (sock, &response.body[wrpos], wrsize - wrpos);
            if (ioresult <= 0)
                break;
            wrpos += ioresult;
            if (wrpos < wrsize) {
                return iocontinue (WRITE_EVENT, KRESPONSE_BODY_LENGTH);
            }
            kont = KRESPONSE_END;
        }
        else if (KRESPONSE_END == kont) {
            if (finalize_response ()) {
                int sock = loop.mplex.fd (handle_id);
                shutdown (sock, SHUT_WR);
                return iocontinue (READ_EVENT, KTEARDOWN);
            }
            kont = KREQUEST_HEADER;
        }
        else if (KTEARDOWN == kont) {
            int sock = loop.mplex.fd (handle_id);
            ioresult = read (sock, &rdbuf[0], BUFFER_SIZE);
            if (ioresult <= 0)
                break;
            if (ioresult > 0) {
                return iocontinue (READ_EVENT, KTEARDOWN);
            }
        }
    }
    return ioresult;
}

void
event_handler_type::prepare_request_body ()
{
    if (reqdecoder.bad ()) {
        message_handler_type h;
        h.bad_request (*this);
        kont = KRESPONSE;
    }
    else {
        response.http_version = request.http_version;
        if (request.header.count ("transfer-encoding") > 0)
            prepare_request_chunked ();
        else if (request.header.count ("content-length") > 0)
            prepare_request_length ();
        else
            kont = KDISPATCH;
    }
}

void
event_handler_type::prepare_request_chunked ()
{
    message_handler_type h;
    kont = KRESPONSE;
    std::vector<std::string> te;
    request.header_token (request.header["transfer-encoding"], te);
    if (te.empty () || te.back () != "chunked")
        h.unsupported_media_type (*this);
    else
        kont = KREQUEST_CHUNKED;
}

void
event_handler_type::finalize_request_chunked ()
{
    message_handler_type h;
    kont = KRESPONSE;
    if (chunkdecoder.bad ())
        h.bad_request (*this);
    else {
        ssize_t const n = chunkdecoder.content_length;
        request.content_length = n;
        request.header["content-length"] = std::to_string (n);
        kont = KDISPATCH;
    }
}

void
event_handler_type::prepare_request_length ()
{
    message_handler_type h;
    kont = KRESPONSE;
    ssize_t n = request.canonical_length (request.header["content-length"]);
    if (-400 == n)
        h.bad_request (*this);
    else if (-413 == n)
        h.request_entity_too_large (*this);
    else if (request.method != "POST" && request.method != "PUT")
        h.request_entity_too_large (*this);
    else {
        request.header["content-length"] = std::to_string (n);
        request.content_length = n;
        kont = KREQUEST_LENGTH;
    }
}

void
event_handler_type::prepare_response ()
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
    guess_close ();
    wrbuf = response.to_string ();
    wrpos = 0;
    wrsize = wrbuf.size ();
}

void
event_handler_type::decide_transfer_encoding ()
{
    if (response.header.count ("transfer-encoding") > 0) {
        std::vector<std::string> te;
        request.header_token (response.header["transfer-encoding"], te);
        if (te.size () != 1 || te.back () != "chunked")
            response.header.erase ("transfer-encoding");
    }
    if (response.header.count ("content-length") > 0) {
        ssize_t n = request.canonical_length (response.header["content-length"]);
        if (n < 0)
            response.header.erase ("content-length");
        else
            response.header["content-length"] = std::to_string (n);
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
event_handler_type::guess_close ()
{
    std::vector<std::string> rqconn;
    if (request.header.count ("connection") > 0)
        request.header_token (request.header["connection"], rqconn);
    if (std::find (rqconn.begin (), rqconn.end (), "close") != rqconn.end ())
        response.header["connection"] = "close";
    else if (request.http_version < "HTTP/1.1"
            && std::find (rqconn.begin (), rqconn.end (), "keep-alive") != rqconn.end ())
        response.header["connection"] = "close";
}

bool
event_handler_type::prepare_response_body ()
{
    if (! response.has_body)
        return false;
    wrpos = 0;
    if (response.chunked) {
        if (response.body_fd < 0)
            response.content_length = response.body.size ();
        response.chunk_size = std::min (
            response.content_length - wrpos, BUFFER_SIZE - 16);
        wrbuf = to_xdigits (response.chunk_size) + "\r\n";
        wrpos1 = 0;
        kont = KRESPONSE_CHUNK_HEADER;
    }
    else if (response.body_fd >= 0) {
        kont = KRESPONSE_FILE_LENGTH;
    }
    else {
        wrsize = response.body.size ();
        kont = KRESPONSE_BODY_LENGTH;
    }
    return true;
}

bool
event_handler_type::finalize_response ()
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
    reqdecoder.clear ();
    return teardown;
}

bool
event_handler_type::done_connection ()
{
    std::vector<std::string> rsconn;
    if (MAX_KEEPALIVE_REQUESTS <= ++keepalive_requests)
        return true;
    if (response.header.count ("connection") > 0)
        request.header_token (response.header["connection"], rsconn);
    if (std::find (rsconn.begin (), rsconn.end (), "close") != rsconn.end ())
        return true;
    if (response.http_version < "HTTP/1.1")
        return std::find (rsconn.begin (), rsconn.end (), "keep-alive") == rsconn.end ();
    return false;
}
