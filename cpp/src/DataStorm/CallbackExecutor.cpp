//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include "CallbackExecutor.h"

using namespace std;
using namespace DataStormI;

CallbackExecutor::CallbackExecutor(function<void(function<void()> call)> executor)
    : _flush(false),
      _destroyed(false),
      _executor(std::move(executor))
{
    _thread = thread(
        [this]
        {
            std::vector<std::pair<std::shared_ptr<DataElementI>, std::function<void()>>> queue;
            while (true)
            {
                unique_lock<mutex> lock(_mutex);
                if (_destroyed)
                {
                    break;
                }
                _cond.wait(lock, [this] { return _flush || _destroyed; });
                if (_flush)
                {
                    _flush = false;
                    _queue.swap(queue);
                }

                lock.unlock();
                for (const auto& p : queue)
                {
                    try
                    {
                        if (_executor)
                        {
                            _executor(p.second);
                        }
                        else
                        {
                            p.second();
                        }
                    }
                    catch (...)
                    {
                        std::terminate();
                    }
                }
                queue.clear();
            }
        });
}

void
CallbackExecutor::queue(const std::shared_ptr<DataElementI>& element, std::function<void()> cb, bool flush)
{
    unique_lock<mutex> lock(_mutex);
    _queue.emplace_back(element, cb);
    if (flush)
    {
        _flush = true;
        _cond.notify_one();
    }
}

void
CallbackExecutor::flush()
{
    unique_lock<mutex> lock(_mutex);
    if (!_queue.empty())
    {
        _flush = true;
        _cond.notify_one();
    }
}

void
CallbackExecutor::destroy()
{
    unique_lock<mutex> lock(_mutex);
    _destroyed = true;
    _cond.notify_one();
    lock.unlock();
    _thread.join();
}
