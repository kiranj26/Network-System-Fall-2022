//
// Created by young on 12/14/21.
//

#ifndef PA4_EVENTLOOP_H
#define PA4_EVENTLOOP_H

#include <set>

#include <sys/epoll.h>


class EventLoop;


class Handler {
public:
    Handler() = default;
    virtual ~Handler() = default;

    virtual bool handle(EventLoop*) = 0;
    [[nodiscard]] virtual int getFD() const = 0;
};


class EventLoop {
public:
    EventLoop();
    virtual ~EventLoop();

    void addHandler(Handler* h);
    void removeHandler(Handler* h);

    void start();
    void stop();
private:
    std::set<Handler*> wait(int);

    int m_epoll_fd;
    bool m_running;
    epoll_event* m_events;
    std::set<Handler*>* mp_active_handlers;
};


#endif //PA4_EVENTLOOP_H
