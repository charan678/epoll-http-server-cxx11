#include <clocale>
#include <cstdlib>
#include <csignal>
#include <cerrno>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "server.hpp"

namespace {
    volatile std::sig_atomic_t g_signal_alrm = 0;
    volatile std::sig_atomic_t g_signal_status = 0;
}

static void
signal_handler (int signal)
{
    if (SIGALRM == signal)
        g_signal_alrm = signal;
    else
        g_signal_status = signal;
}

static void
set_signal_handler (int const sig, void (*handler)(int), int const flags)
{
    struct sigaction sa;
    sa.sa_handler = handler;
    sa.sa_flags = flags;
    sigemptyset (&sa.sa_mask);
    sigaction (sig, &sa, nullptr);
}

static void
start_interval_timer (long const sec, long const usec)
{
    logger_type& log = logger_type::getinstance ();
    struct itimerval timer;
    timer.it_interval.tv_sec = sec;
    timer.it_interval.tv_usec = usec;
    timer.it_value.tv_sec = sec;
    timer.it_value.tv_usec = usec;
    if (setitimer (ITIMER_REAL, &timer, nullptr) < 0) {
        log.put_error ("setitimer");
        exit (EXIT_FAILURE);
    }
}

int
main (int argc, char *argv[])
{
    std::setlocale (LC_ALL, "C");
    std::signal (SIGPIPE, SIG_IGN);
    set_signal_handler (SIGINT, signal_handler, 0);
    set_signal_handler (SIGALRM, signal_handler, 0);
    start_interval_timer (1L, 0);
    epoll_mplex_type mplex (LISTENER_COUNT + MAX_CONNECTIONS);
    tcpserver_type server (MAX_CONNECTIONS, TIMEOUT, mplex);
    server.run (SERVER_PORT, BACKLOG);
    return EXIT_SUCCESS;
}

static inline struct sockaddr *
sockaddr_ptr (struct sockaddr_in& addr)
{
    return reinterpret_cast<struct sockaddr *> (&addr);
}

int
tcpserver_type::initialize (int const port, int const backlog)
{
    logger_type& log = logger_type::getinstance ();
    listen_port = port;
    listen_sock = listen_socket_create (port, backlog);
    if (listen_sock < 0)
        ;
    else if (fd_set_nonblock (listen_sock) < 0)
        log.put_error ("fd_set_nonblock (listen_fd)");
    else if (mplex.add (READ_EVENT, listen_sock, 0) < 0)
        ;
    else {
        log.put_info ("listening port " + std::to_string (port));
        return RUN;
    }
    if (listen_sock >= 0) {
        close (listen_sock);
        listen_sock = -1;
    }
    return STOP;
}

void
tcpserver_type::run (int const port, int const backlog)
{
    logger_type& log = logger_type::getinstance ();
    handlers.resize (1 + max_connections);
    handlers.erase (WAIT);
    int kont = initialize (port, backlog);
    while (RUN == kont) {
        if (g_signal_alrm) {
            mplex.run_timer ();
            g_signal_alrm = 0;
        }
        else if (mplex.wait (1000) < 0) {
            log.put_error ("mplex.wait");
            break;
        }
        if (g_signal_status)
            break;
        if (mplex.empty ())
            continue;
        std::size_t next_i = mplex.end ();
        for (std::size_t i = mplex.begin (); i != mplex.end (); i = next_i) {
            if (g_signal_status)
                break;
            next_i = mplex.next (i);
            uint32_t events = mplex.events (i);
            int handler_id = mplex.handler_id (i);
            if (events & TIMER_EVENT) {
                mplex.stop_timer (i);
                handlers[handler_id].on_timer (*this);
            }
            else if ((events & READ_EVENT) && 0 < handler_id) {
                handlers[handler_id].on_read (*this);
            }
            else if (events & WRITE_EVENT) {
                handlers[handler_id].on_write (*this);
            }
            else if ((events & READ_EVENT) && 0 == handler_id) {
                if (! handlers.empty (FREE)) {
                    int fresh_handler_id = handlers[FREE].next;
                    handlers[fresh_handler_id].on_accept (*this);
                }
            }
        }
    }
    shutdown ();
}

int
tcpserver_type::register_handler (std::size_t handler_id)
{
    if (FREE == handlers[handler_id].state) {
        handlers[handler_id].state = WAIT;
        handlers.erase (handler_id);
        handlers.insert (handlers[handler_id].state, handler_id);
    }
    return WAIT;
}

int
tcpserver_type::remove_handler (std::size_t handler_id)
{
    handlers[handler_id].on_close (*this);
    if (FREE != handlers[handler_id].state) {
        handlers[handler_id].state = FREE;
        handlers.erase (handler_id);
        handlers.insert (handlers[handler_id].state, handler_id);
    }
    return FREE;
}

void
tcpserver_type::shutdown ()
{
    logger_type& log = logger_type::getinstance ();
    log.put_info ("shutdown");
    for (std::size_t i = 0; i < handlers.size (); ++i) {
        if (handlers[i].id > 0 && FREE != handlers[i].state)
            handlers[i].on_close (*this);
    }
    if (listen_sock >= 0) {
        close (listen_sock);
        listen_sock = -1;
    }
}

int
tcpserver_type::listen_socket_create (int const port, int const backlog)
{
    logger_type& log = logger_type::getinstance ();
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl (INADDR_ANY);
    addr.sin_port = htons (port);
    int yes = 1;
    int const sock = socket (PF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        log.put_error ("socket");
    else if (setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) < 0)
        log.put_error ("setsockopt");
    else if (bind (sock, sockaddr_ptr (addr), sizeof addr) < 0)
        log.put_error ("bind");
    else if (listen (sock, backlog) < 0)
        log.put_error ("listen");
    else
        return sock;
    if (sock >= 0)
        close (sock);
    return -1;
}

int
tcpserver_type::accept_client (std::string& remote_addr)
{
    logger_type& log = logger_type::getinstance ();
    struct sockaddr_in addr;
    socklen_t len = sizeof addr;
    int const conn_sock = accept (listen_sock, sockaddr_ptr (addr), &len);
    int const e = errno;
    if (conn_sock < 0 && (EINTR == e || EAGAIN == e || EWOULDBLOCK == e))
        ;
    else if (conn_sock < 0)
        log.put_error ("accept");
    else
        remote_addr = inet_ntoa (addr.sin_addr);
    return conn_sock;
}

int
tcpserver_type::fd_set_nonblock (int fd)
{
    int flag;
    if ((flag = fcntl (fd, F_GETFL, 0)) >= 0)
        flag = fcntl (fd, F_SETFL, flag | O_NONBLOCK);
    return flag;
}
