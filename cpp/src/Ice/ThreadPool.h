//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef ICE_THREAD_POOL_H
#define ICE_THREAD_POOL_H

#include <IceUtil/Mutex.h>
#include <IceUtil/Monitor.h>
#include <IceUtil/Thread.h>

#include <Ice/Config.h>
#include <Ice/Dispatcher.h>
#include <Ice/ThreadPoolF.h>
#include <Ice/InstanceF.h>
#include <Ice/LoggerF.h>
#include <Ice/PropertiesF.h>
#include <Ice/EventHandler.h>
#include <Ice/Selector.h>
#include <Ice/InputStream.h>
#include <Ice/ObserverHelper.h>

#include <set>
#include <list>

namespace IceInternal
{

class ThreadPoolCurrent;

class ThreadPoolWorkQueue;
using ThreadPoolWorkQueuePtr = std::shared_ptr<ThreadPoolWorkQueue>;

class ThreadPoolWorkItem
{
public:

    virtual void execute(ThreadPoolCurrent&) = 0;
};
using ThreadPoolWorkItemPtr = std::shared_ptr<ThreadPoolWorkItem>;

class DispatchWorkItem :
    public ThreadPoolWorkItem,
    public Ice::DispatcherCall,
    public std::enable_shared_from_this<DispatchWorkItem>
{
public:

    DispatchWorkItem();
    DispatchWorkItem(const Ice::ConnectionPtr& connection);

    const Ice::ConnectionPtr&
    getConnection()
    {
        return _connection;
    }

private:

    virtual void execute(ThreadPoolCurrent&);

    const Ice::ConnectionPtr _connection;
};
using DispatchWorkItemPtr = std::shared_ptr<DispatchWorkItem>;

class ThreadPool : public std::enable_shared_from_this<ThreadPool>
{
    class EventHandlerThread : public IceUtil::Thread
    {
    public:

        EventHandlerThread(const ThreadPoolPtr&, const std::string&);
        virtual void run();

        void updateObserver();
        void setState(Ice::Instrumentation::ThreadState);

    private:

        ThreadPoolPtr _pool;
        ObserverHelperT<Ice::Instrumentation::ThreadObserver> _observer;
        Ice::Instrumentation::ThreadState _state;
    };
    using EventHandlerThreadPtr = std::shared_ptr<EventHandlerThread>;

public:

    static ThreadPoolPtr create(const InstancePtr&, const std::string&, int);

    virtual ~ThreadPool();

    void destroy();

    void updateObservers();

    void initialize(const EventHandlerPtr&);
    void _register(const EventHandlerPtr& handler, SocketOperation status)
    {
        update(handler, SocketOperationNone, status);
    }
    void update(const EventHandlerPtr&, SocketOperation, SocketOperation);
    void unregister(const EventHandlerPtr& handler, SocketOperation status)
    {
        update(handler, status, SocketOperationNone);
    }
    bool finish(const EventHandlerPtr&, bool);
    void ready(const EventHandlerPtr&, SocketOperation, bool);

    void dispatchFromThisThread(const DispatchWorkItemPtr&);
    void dispatchFromThisThread(Ice::ConnectionPtr, std::function<void()>);
    void dispatch(const DispatchWorkItemPtr&);
    void dispatch(std::function<void()>);

    void joinWithAllThreads();

    std::string prefix() const;

#ifdef ICE_SWIFT
    dispatch_queue_t getDispatchQueue() const noexcept;
#endif

private:

    ThreadPool(const InstancePtr&, const std::string&, int);
    void initialize();

    void run(const EventHandlerThreadPtr&);

    bool ioCompleted(ThreadPoolCurrent&);

#if defined(ICE_USE_IOCP)
    bool startMessage(ThreadPoolCurrent&);
    void finishMessage(ThreadPoolCurrent&);
#else
    void promoteFollower(ThreadPoolCurrent&);
    bool followerWait(ThreadPoolCurrent&, std::unique_lock<std::mutex>&);
#endif

    std::string nextThreadId();

    const InstancePtr _instance;
#ifdef ICE_SWIFT
    const dispatch_queue_t _dispatchQueue;
#else // Ice for Swift does not support a dispatcher
    std::function<void(std::function<void()>, const std::shared_ptr<Ice::Connection>&)> _dispatcher;
#endif
    ThreadPoolWorkQueuePtr _workQueue;
    bool _destroyed;
    const std::string _prefix;
    Selector _selector;
    int _nextThreadId;

    friend class EventHandlerThread;
    friend class ThreadPoolCurrent;
    friend class ThreadPoolWorkQueue;

    const int _size; // Number of threads that are pre-created.
    const int _sizeIO; // Maximum number of threads that can concurrently perform IO.
    const int _sizeMax; // Maximum number of threads.
    const int _sizeWarn; // If _inUse reaches _sizeWarn, a "low on threads" warning will be printed.
    const bool _serialize; // True if requests need to be serialized over the connection.
    const bool _hasPriority;
    const int _priority;
    const int _serverIdleTime;
    const int _threadIdleTime;
    const size_t _stackSize;

    std::set<EventHandlerThreadPtr> _threads; // All threads, running or not.
    int _inUse; // Number of threads that are currently in use.
#if !defined(ICE_USE_IOCP)
    int _inUseIO; // Number of threads that are currently performing IO.
    std::vector<std::pair<EventHandler*, SocketOperation> > _handlers;
    std::vector<std::pair<EventHandler*, SocketOperation> >::const_iterator _nextHandler;
#endif

    bool _promote;
    std::mutex _mutex;
    std::condition_variable _conditionVariable;
};

class ThreadPoolCurrent
{
public:

    ThreadPoolCurrent(const InstancePtr&, const ThreadPoolPtr&, const ThreadPool::EventHandlerThreadPtr&);

    SocketOperation operation;
    Ice::InputStream stream; // A per-thread stream to be used by event handlers for optimization.

    bool ioCompleted() const
    {
        return _threadPool->ioCompleted(const_cast<ThreadPoolCurrent&>(*this));
    }

#if defined(ICE_USE_IOCP)
    bool startMessage()
    {
        return _threadPool->startMessage(const_cast<ThreadPoolCurrent&>(*this));
    }

    void finishMessage()
    {
        _threadPool->finishMessage(const_cast<ThreadPoolCurrent&>(*this));
    }
#else
    bool ioReady()
    {
        return (_handler->_registered & operation) != 0;
    }
#endif

    void dispatchFromThisThread(const DispatchWorkItemPtr& workItem)
    {
        _threadPool->dispatchFromThisThread(workItem);
    }

private:

    ThreadPool* _threadPool;
    ThreadPool::EventHandlerThreadPtr _thread;
    EventHandlerPtr _handler;
    bool _ioCompleted;
#if !defined(ICE_USE_IOCP)
    bool _leader;
#else
    DWORD _count;
    int _error;
#endif
    friend class ThreadPool;
};

class ThreadPoolWorkQueue : public EventHandler
{
public:

    ThreadPoolWorkQueue(ThreadPool&);

    void destroy();
    void queue(const ThreadPoolWorkItemPtr&);

#if defined(ICE_USE_IOCP)
    bool startAsync(SocketOperation);
    bool finishAsync(SocketOperation);
#endif

    virtual void message(ThreadPoolCurrent&);
    virtual void finished(ThreadPoolCurrent&, bool);
    virtual std::string toString() const;
    virtual NativeInfoPtr getNativeInfo();

private:

    ThreadPool& _threadPool;
    bool _destroyed;
    std::list<ThreadPoolWorkItemPtr> _workItems;
};

//
// The ThreadPoolMessage class below hides the IOCP implementation details from
// the event handler implementations. Only event handler implementation that
// require IO need to use this class.
//
// An instance of the IOScope subclass must be created within the synchronization
// of the event handler. It takes care of calling startMessage/finishMessage for
// the IOCP implementation and ensures that finishMessage isn't called multiple
// times.
//
#if !defined(ICE_USE_IOCP)
template<class T> class ThreadPoolMessage
{
public:

    class IOScope
    {
    public:

        IOScope(ThreadPoolMessage<T>& message) : _message(message)
        {
            // Nothing to do.
        }

        ~IOScope()
        {
            // Nothing to do.
        }

        operator bool()
        {
            return _message._current.ioReady(); // Ensure the handler is still interested in the operation.
        }

        void completed()
        {
            _message._current.ioCompleted();
        }

    private:

        ThreadPoolMessage<T>& _message;
    };
    friend class IOScope;

    ThreadPoolMessage(ThreadPoolCurrent& current, const T&) : _current(current)
    {
    }

    ~ThreadPoolMessage()
    {
        // Nothing to do.
    }

private:

    ThreadPoolCurrent& _current;
};

#else

template<class T> class ThreadPoolMessage
{
public:

    class IOScope
    {
    public:

        IOScope(ThreadPoolMessage& message) : _message(message)
        {
            // This must be called with the handler locked.
            _finish = _message._current.startMessage();
        }

        ~IOScope()
        {
            if(_finish)
            {
                // This must be called with the handler locked.
                _message._current.finishMessage();
            }
        }

        operator bool()
        {
            return _finish;
        }

        void
        completed()
        {
            //
            // Call finishMessage once IO is completed only if serialization is not enabled.
            // Otherwise, finishMessage will be called when the event handler is done with
            // the message (it will be called from ~ThreadPoolMessage below).
            //
            assert(_finish);
            if(_message._current.ioCompleted())
            {
                _finish = false;
                _message._finish = true;
            }
        }

    private:

        ThreadPoolMessage& _message;
        bool _finish;
    };
    friend class IOScope;

    ThreadPoolMessage(ThreadPoolCurrent& current, const T& eventHandler) :
        _current(current),
        _eventHandler(eventHandler),
        _finish(false)
    {
    }

    ~ThreadPoolMessage()
    {
        if(_finish)
        {
            //
            // A ThreadPoolMessage instance must be created outside the synchronization
            // of the event handler. We need to lock the event handler here to call
            // finishMessage.
            //
            lock_guard lock(_eventHandler._mutex);
            _current.finishMessage();
        }
    }

private:

    ThreadPoolCurrent& _current;
    const T& _eventHandler;
    bool _finish;
};
#endif

};

#endif
