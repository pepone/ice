//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include <IceUtil/RecMutex.h>
#include <IceUtil/Exception.h>

using namespace std;

IceUtil::RecMutex::RecMutex() :
    _count(0)
{
    init();
}

#ifdef _WIN32

void
IceUtil::RecMutex::init()
{
    InitializeCriticalSection(&_mutex);
}

IceUtil::RecMutex::~RecMutex()
{
    assert(_count == 0);
    DeleteCriticalSection(&_mutex);
}

void
IceUtil::RecMutex::lock() const
{
    EnterCriticalSection(&_mutex);
    if(++_count > 1)
    {
        LeaveCriticalSection(&_mutex);
    }
}

bool
IceUtil::RecMutex::tryLock() const
{
    if(!TryEnterCriticalSection(&_mutex))
    {
        return false;
    }
    if(++_count > 1)
    {
        LeaveCriticalSection(&_mutex);
    }
    return true;
}

void
IceUtil::RecMutex::unlock() const
{
    if(--_count == 0)
    {
        LeaveCriticalSection(&_mutex);
    }
}

#   ifdef ICE_HAS_WIN32_CONDVAR
void
IceUtil::RecMutex::unlock(LockState& state) const
{
    state.mutex = &_mutex;
    state.count = _count;
    _count = 0;
}

void
IceUtil::RecMutex::lock(LockState& state) const
{
    _count = state.count;
}
#   else
void
IceUtil::RecMutex::unlock(LockState& state) const
{
    state.count = _count;
    _count = 0;
    LeaveCriticalSection(&_mutex);
}

void
IceUtil::RecMutex::lock(LockState& state) const
{
    EnterCriticalSection(&_mutex);
    _count = state.count;
}
#   endif

#else

void
IceUtil::RecMutex::init()
{
    int rc;
    pthread_mutexattr_t attr;
    rc = pthread_mutexattr_init(&attr);
    assert(rc == 0);
    if(rc != 0)
    {
        pthread_mutexattr_destroy(&attr);
        throw ThreadSyscallException(__FILE__, __LINE__, rc);
    }

    rc = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    assert(rc == 0);
    if(rc != 0)
    {
        pthread_mutexattr_destroy(&attr);
        throw ThreadSyscallException(__FILE__, __LINE__, rc);
    }

    rc = pthread_mutex_init(&_mutex, &attr);
    assert(rc == 0);
    if(rc != 0)
    {
        pthread_mutexattr_destroy(&attr);
        throw ThreadSyscallException(__FILE__, __LINE__, rc);
    }

    rc = pthread_mutexattr_destroy(&attr);
    assert(rc == 0);
    if(rc != 0)
    {
        throw ThreadSyscallException(__FILE__, __LINE__, rc);
    }
}

IceUtil::RecMutex::~RecMutex()
{
    assert(_count == 0);
#ifndef NDEBUG
    int rc = pthread_mutex_destroy(&_mutex);
    assert(rc == 0);
#else
    pthread_mutex_destroy(&_mutex);
#endif
}

void
IceUtil::RecMutex::lock() const
{
    int rc = pthread_mutex_lock(&_mutex);
    if(rc != 0)
    {
        throw ThreadSyscallException(__FILE__, __LINE__, rc);
    }
    if(++_count > 1)
    {
        rc = pthread_mutex_unlock(&_mutex);
        assert(rc == 0);
    }
}

bool
IceUtil::RecMutex::tryLock() const
{
    int rc = pthread_mutex_trylock(&_mutex);
    bool result = (rc == 0);
    if(!result)
    {
        if(rc != EBUSY)
        {
            throw ThreadSyscallException(__FILE__, __LINE__, rc);
        }
    }
    else if(++_count > 1)
    {
        rc = pthread_mutex_unlock(&_mutex);
        if(rc != 0)
        {
            throw ThreadSyscallException(__FILE__, __LINE__, rc);
        }
    }
    return result;
}

void
IceUtil::RecMutex::unlock() const
{
    if(--_count == 0)
    {
#ifndef NDEBUG
        int rc = pthread_mutex_unlock(&_mutex);
        assert(rc == 0);
#else
        pthread_mutex_unlock(&_mutex);
#endif
    }
}

void
IceUtil::RecMutex::unlock(LockState& state) const
{
    state.mutex = &_mutex;
    state.count = _count;
    _count = 0;
}

void
IceUtil::RecMutex::lock(LockState& state) const
{
    _count = state.count;
}

#endif

bool
IceUtil::RecMutex::willUnlock() const
{
    return _count == 1;
}
