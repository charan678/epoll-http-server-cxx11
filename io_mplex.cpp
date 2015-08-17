#include <ctime>
#include <cerrno>
#include "server.hpp"

io_mplex_type::io_mplex_type (std::size_t n)
    : max_events (n), handles (), timers ()
{
    handles.resize (n + 3);
    handles.erase (WAIT);
    handles.erase (READY);
}

int
io_mplex_type::add_timer (std::time_t const uptime, std::size_t const handler_id)
{
    if (handles.empty (FREE)) {
        errno = ENOMEM;
        return -1;
    }
    std::time_t looptime = std::time (nullptr);
    std::size_t const id = handles[FREE].next;
    handles[id].ev_permit = false;
    handles[id].fd = -1;
    handles[id].uptime = uptime;
    handles[id].handler_id = handler_id;
    handles[id].state = looptime < uptime ? WAIT : READY;
    handles[id].ev_mask = TIMER_EVENT;
    handles[id].events = looptime < uptime ? 0 : TIMER_EVENT;
    handles.erase (id);
    handles.insert (handles[id].state, id);
    if (WAIT == handles[id].state)
        timers.emplace (uptime, id);
    return id;
}

int
io_mplex_type::mod_timer (std::time_t const uptime, std::size_t const id)
{
    if (range_check (id) < 0)
        return -1;
    std::size_t const next_id = handles[id].next;
    stop_timer (id);
    std::time_t looptime = std::time (nullptr);
    int prev_state = handles[id].state;
    handles[id].uptime = uptime;
    handles[id].state = looptime < uptime ? handles[id].state : READY;
    handles[id].ev_mask |= TIMER_EVENT;
    handles[id].events |= looptime < uptime ? 0 : TIMER_EVENT;
    if (prev_state != handles[id].state) {
        handles.erase (id);
        handles.insert (handles[id].state, id);
    }
    if (WAIT == handles[id].state)
        timers.emplace (uptime, id);
    return handles[next_id].prev;
}

int
io_mplex_type::stop_timer (std::size_t const id)
{
    if (range_check (id) < 0)
        return -1;
    std::size_t const next_id = handles[id].next;
    handles[id].uptime = 0;
    handles[id].events &= ~TIMER_EVENT;
    if (TIMER_EVENT & handles[id].ev_mask) {
        auto i = timers.lower_bound (handles[id].uptime);
        for (; i != timers.end (); ++i) {
            if (i->first > handles[id].uptime)
                break;
            else if (i->second == id) {
                timers.erase (i);
                break;
            }
        }
    }
    handles[id].ev_mask &= ~TIMER_EVENT;
    return handles[next_id].prev;
}

void
io_mplex_type::run_timer ()
{
    std::time_t looptime = std::time (nullptr);
    for (auto i = timers.begin (); i != timers.end () && i->first <= looptime;) {
        handles[i->second].events |= TIMER_EVENT;
        if (WAIT == handles[i->second].state) {
            handles.erase (i->second);
            handles.insert (READY, i->second);
        }
        i = timers.erase (i);
    }
}