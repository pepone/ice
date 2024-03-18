//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef ICE_UTIL_TIMER_H
#define ICE_UTIL_TIMER_H

#include <IceUtil/Exception.h>

#include <set>
#include <map>
#include <mutex>
#include <chrono>
#include <functional>
#include <optional>
#include <stdexcept>

namespace IceUtil
{
    class Timer;
    using TimerPtr = std::shared_ptr<Timer>;

    class ICE_API TimerTask
    {
    public:
        virtual ~TimerTask();

        virtual void runTimerTask() = 0;
    };
    using TimerTaskPtr = std::shared_ptr<TimerTask>;

    // The timer class is used to schedule tasks for one-time execution or repeated execution. Tasks are executed by a
    // dedicated timer thread sequentially.
    class ICE_API Timer
    {
    public:
        Timer();
        virtual ~Timer() = default;

        // Destroy the timer and detach its execution thread if the calling thread
        // is the timer thread, join the timer execution thread otherwise.
        void destroy();

        // Schedule a task for execution after a given delay.
        template<class Rep, class Period>
        void schedule(const TimerTaskPtr& task, const std::chrono::duration<Rep, Period>& delay)
        {
            std::lock_guard lock(_mutex);
            if (_destroyed)
            {
                throw std::invalid_argument("timer destroyed");
            }

            if (delay < std::chrono::nanoseconds::zero())
            {
                throw std::invalid_argument("invalid negative delay");
            }

            auto now = std::chrono::steady_clock::now();
            auto time = now + delay;
            if (delay > std::chrono::nanoseconds::zero() && time < now)
            {
                throw std::invalid_argument("delay too large, resulting in overflow");
            }

            bool inserted = _tasks.insert(make_pair(task, time)).second;
            if (!inserted)
            {
                throw std::invalid_argument("task is already scheduled");
            }
            _tokens.insert({time, std::nullopt, task});

            if (_wakeUpTime == std::chrono::steady_clock::time_point() || time < _wakeUpTime)
            {
                _condition.notify_one();
            }
        }

        // Schedule a task for repeated execution with the given delay between each execution.
        template<class Rep, class Period>
        void scheduleRepeated(TimerTaskPtr task, const std::chrono::duration<Rep, Period>& delay)
        {
            std::lock_guard lock(_mutex);
            if (_destroyed)
            {
                throw std::invalid_argument("timer destroyed");
            }

            if (delay < std::chrono::nanoseconds::zero())
            {
                throw std::invalid_argument("invalid negative delay");
            }

            auto now = std::chrono::steady_clock::now();
            auto time = now + delay;
            if (delay > std::chrono::nanoseconds::zero() && time < now)
            {
                throw std::invalid_argument("delay too large, resulting in overflow");
            }

            bool inserted = _tasks.insert(make_pair(task, time)).second;
            if (!inserted)
            {
                throw std::invalid_argument("task is already scheduled");
            }
            _tokens.insert({time, std::chrono::duration_cast<std::chrono::nanoseconds>(delay), task});

            if (_wakeUpTime == std::chrono::steady_clock::time_point() || time < _wakeUpTime)
            {
                _condition.notify_one();
            }
        }

        //
        // Cancel a task. Returns true if the task has not yet run or if
        // it's a task scheduled for repeated execution. Returns false if
        // the task has already run, was already cancelled or was never
        // scheduled.
        //
        bool cancel(const TimerTaskPtr&);

    protected:
        virtual void runTimerTask(const IceUtil::TimerTaskPtr&);

    private:
        struct Token
        {
            std::chrono::steady_clock::time_point scheduledTime;
            std::optional<std::chrono::nanoseconds> delay;
            TimerTaskPtr task;
            bool operator<(const Token& other) const
            {
                if (scheduledTime < other.scheduledTime)
                {
                    return true;
                }
                else if (scheduledTime > other.scheduledTime)
                {
                    return false;
                }
                return task < other.task;
            }
        };

        void run();

        std::mutex _mutex;
        std::condition_variable _condition;
        std::set<Token> _tokens;
        std::map<TimerTaskPtr, std::chrono::steady_clock::time_point> _tasks;
        bool _destroyed;
        std::chrono::steady_clock::time_point _wakeUpTime;
        std::thread _worker;
    };
}

#endif
