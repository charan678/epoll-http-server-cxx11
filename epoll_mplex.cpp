#include <cerrno>
#include <unistd.h>
#include <sys/epoll.h>
#include "server.hpp"

epoll_mplex_type::epoll_mplex_type (std::size_t n)
    : io_mplex_type (n), epoll_fd (-1), evset ()
{
    evset = new struct epoll_event[n];
    epoll_fd = epoll_create (n);
}

epoll_mplex_type::~epoll_mplex_type ()
{
    close (epoll_fd);
    delete[] evset;
    evset = nullptr;
}

int
epoll_mplex_type::add (uint32_t const trigger, int const fd, std::size_t const handler_id)
{
    if (fd < 0) {
        errno = EBADF;
        return fd;
    }
    if (handles.empty (FREE)) {
        errno = ENOMEM;
        return -1;
    }
    std::size_t const id = handles[FREE].next;
    bool ev_permit = true;
    int epck = epoll_create (1);
    struct epoll_event ev;
    ev.events = 0;
    ev.events |= trigger & READ_EVENT ? EPOLLIN : 0;
    ev.events |= trigger & WRITE_EVENT ? EPOLLOUT : 0;
    ev.events |= trigger & EDGE_EVENT ? EPOLLET : 0;
    ev.data.u32 = id;
    if (epoll_ctl (epck, EPOLL_CTL_ADD, fd, &ev) < 0) {
        if (EPERM == errno)
            ev_permit = false;
        else
            epoll_ctl (epck, EPOLL_CTL_DEL, fd, &ev);
    }
    close (epck);
    if (ev_permit && epoll_ctl (epoll_fd, EPOLL_CTL_ADD, fd, &ev) < 0)
        return -1;
    handles[id].ev_permit = ev_permit;
    handles[id].fd = fd;
    handles[id].uptime = 0;
    handles[id].handler_id = handler_id;
    handles[id].state = ev_permit ? WAIT : READY;
    handles[id].ev_mask = trigger & (READ_EVENT|WRITE_EVENT);
    handles[id].events = ev_permit ? 0 : (trigger & (READ_EVENT|WRITE_EVENT));
    handles.erase (id);
    handles.insert (handles[id].state, id);
    return id;
}

int
epoll_mplex_type::drop (uint32_t const events, std::size_t const id)
{
    if (range_check (id) < 0)
        return -1;
    std::size_t const next_id = handles[id].next;
    if (handles[id].ev_permit)
        handles[id].events &= ~events;
    if (READY == handles[id].state && ! (handles[id].events & handles[id].ev_mask)) {
        handles[id].state = WAIT;
        handles.erase (id);
        handles.insert (handles[id].state, id);
    }
    return handles[next_id].prev;
}

int
epoll_mplex_type::mod (uint32_t const trigger, std::size_t id)
{
    struct epoll_event ev;
    if (range_check (id) < 0)
        return -1;
    std::size_t next_id = handles[id].next;
    handles[id].ev_mask &= ~(READ_EVENT|WRITE_EVENT);
    handles[id].ev_mask |= (trigger & (READ_EVENT|WRITE_EVENT));
    if (trigger & (READ_EVENT|WRITE_EVENT)) {
        if (handles[id].ev_permit) {
            ev.events = 0;
            ev.events |= trigger & READ_EVENT ? EPOLLIN : 0;
            ev.events |= trigger & WRITE_EVENT ? EPOLLOUT : 0;
            ev.events |= trigger & EDGE_EVENT ? EPOLLET : 0;
            ev.data.u32 = id;
            if (epoll_ctl (epoll_fd, EPOLL_CTL_MOD, handles[id].fd, &ev) < 0)
                return -1;
        }
        else {
            handles[id].events &= ~(READ_EVENT|WRITE_EVENT);
            handles[id].events |= trigger & (READ_EVENT|WRITE_EVENT);
        }
    }
    else {
        if (handles[id].ev_permit) {
            if (epoll_ctl (epoll_fd, EPOLL_CTL_DEL, handles[id].fd, &ev) < 0)
                return -1;
        }
    }
    int prev_state = handles[id].state;
    if (handles[id].ev_mask & handles[id].events)
        handles[id].state = READY;
    else
        handles[id].state = WAIT;
    if (prev_state != handles[id].state) {
        handles.erase (id);
        handles.insert (handles[id].state, id);
    }
    return handles[next_id].prev;
}

int
epoll_mplex_type::del (std::size_t const id)
{
    struct epoll_event ev;
    if (range_check (id) < 0)
        return -1;
    std::size_t const next_id = handles[id].next;
    if (handles[id].ev_permit) {
        if (epoll_ctl (epoll_fd, EPOLL_CTL_DEL, handles[id].fd, &ev) < 0)
            return -1;
    }
    stop_timer (id);
    handles[id].ev_permit = false;
    handles[id].fd = -1;
    handles[id].handler_id = 0;
    handles[id].state = FREE;
    handles[id].ev_mask = 0;
    handles[id].events = 0;
    handles.erase (id);
    handles.insert (handles[id].state, id);
    return handles[next_id].prev;
}

int
epoll_mplex_type::wait (int msec)
{
    run_timer ();
    if (! handles.empty (READY))
        msec = 0;
    int const nevent = epoll_wait (epoll_fd, evset, max_events, msec);
    if (nevent < 0 && EINTR == errno)
        return 0;
    if (nevent < 0)
        return nevent;
    for (int i = 0; i < nevent; ++i) {
        uint32_t const ep_events = evset[i].events;
        uint32_t events = 0;
        events |= ep_events & EPOLLIN ? READ_EVENT : 0;
        events |= ep_events & EPOLLOUT ? WRITE_EVENT : 0;
        int const id = evset[i].data.u32;
        handles[id].events |= events;
        if (WAIT == handles[id].state && (handles[id].ev_mask & handles[id].events)) {
            handles.erase (id);
            handles.insert (READY, id);
        }
    }
    return nevent;
}
