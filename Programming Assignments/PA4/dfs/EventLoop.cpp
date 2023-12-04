//
// Created by young on 12/14/21.
//

#include <iostream>
#include <unistd.h>

#include "EventLoop.h"


EventLoop::EventLoop() {
    m_epoll_fd = epoll_create1(0);
    m_running = true;

    m_events = (struct epoll_event*)malloc(sizeof(struct epoll_event) * 30);

    mp_active_handlers = new std::set<Handler*>;
}

EventLoop::~EventLoop() {
    // mp_timer_handler is destroyed here
    // along with other HttpHandler and Server
    for (auto h: *mp_active_handlers) {
        delete h;
    }

    // when the set is cleared.
    mp_active_handlers->clear();

    close(m_epoll_fd);
    free(m_events);
    delete mp_active_handlers;
}


void EventLoop::addHandler(Handler* handler)
{
    struct epoll_event event{};
    event.events = EPOLLIN;
    event.data.ptr = reinterpret_cast<void*>(handler);
    epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, handler->getFD(), &event);

    mp_active_handlers->insert(handler);
}


void EventLoop::removeHandler(Handler* handler) {
    epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, handler->getFD(), nullptr);

    try {
//        std::cout << "# of active handlers: " << mp_active_handlers->size() << std::endl;
        mp_active_handlers->erase(handler);
        std::cout << "# of active handlers: " << mp_active_handlers->size() << std::endl;
        close(handler->getFD());
    } catch (std::invalid_argument& ia) {
        std::cerr << ia.what() << std::endl;
    }
}


std::set<Handler*> EventLoop::wait(int timeout)
{
    std::set<Handler*> read_ready_handlers;
    int count = epoll_wait(m_epoll_fd, m_events, 30, timeout);

    for (int i = 0; i < count; i++)
    {
        read_ready_handlers.insert((Handler*)m_events[i].data.ptr);
    }

    return read_ready_handlers;
}


void EventLoop::start()
{
    while (m_running)
    {
        std::set<Handler*> handlers = wait(1000);

        for (auto handler: handlers)
        {
            // handler could be Server, HttpHandler, or TimerHandler
            bool isDone = handler->handle(this);

            if (isDone)
            {
                removeHandler(handler);
                delete handler;
            }
        }
    }
}

void EventLoop::stop()
{
    m_running = false;
}