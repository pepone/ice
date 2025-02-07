// Copyright (c) ZeroC, Inc.

#include "LoggerMiddleware.h"
#include "Ice/StringUtil.h"
#include "Ice/UserException.h"

using namespace std;
using namespace Ice;
using namespace IceInternal;

LoggerMiddleware::LoggerMiddleware(
    Ice::ObjectPtr next,
    LoggerPtr logger,
    int traceLevel,
    const char* traceCat,
    int warningLevel,
    ToStringMode toStringMode)
    : _next(std::move(next)),
      _logger(std::move(logger)),
      _traceLevel(traceLevel),
      _traceCat(traceCat),
      _warningLevel(warningLevel),
      _toStringMode(toStringMode)
{
    assert(_next);
    assert(_logger);
    assert(_traceLevel > 0 || _warningLevel > 0);
}

void
LoggerMiddleware::dispatch(Ice::IncomingRequest& request, function<void(OutgoingResponse)> sendResponse)
{
    try
    {
        _next->dispatch(
            request,
            [sendResponse = std::move(sendResponse), self = shared_from_this()](OutgoingResponse response)
            {
                switch (response.replyStatus())
                {
                    case ReplyStatus::Ok:
                    case ReplyStatus::UserException:
                        if (self->_traceLevel > 0)
                        {
                            self->logDispatch(response.replyStatus(), response.current());
                        }
                        break;
                    case ReplyStatus::ObjectNotExist:
                    case ReplyStatus::FacetNotExist:
                    case ReplyStatus::OperationNotExist:
                        if (self->_traceLevel > 0 || self->_warningLevel > 1)
                        {
                            self->logDispatchException(response.exceptionDetails(), response.current());
                        }
                        break;

                    default:
                        self->logDispatchException(response.exceptionDetails(), response.current());
                        break;
                }
                sendResponse(std::move(response));
            });
    }
    catch (const UserException&)
    {
        if (_traceLevel > 0)
        {
            logDispatch(ReplyStatus::UserException, request.current());
        }
        throw;
    }
    catch (const RequestFailedException& ex)
    {
        if (_traceLevel > 0 || _warningLevel > 1)
        {
            logDispatchException(ex, request.current());
        }
        throw;
    }
    catch (const Ice::LocalException& ex)
    {
        logDispatchException(ex, request.current());
        throw;
    }
    catch (const std::exception& ex)
    {
        logDispatchException(ex.what(), request.current());
        throw;
    }
    catch (...)
    {
        logDispatchException("c++ exception", request.current());
        throw;
    }
}

void
LoggerMiddleware::logDispatch(ReplyStatus replyStatus, const Current& current) const noexcept
{
    Trace out{_logger, _traceCat};
    out << "dispatch of " << current.operation << " to ";
    printTarget(out, current);
    out << " returned a response status with reply status " << replyStatus;
}

void
LoggerMiddleware::logDispatchException(string_view exceptionDetails, const Current& current) const noexcept
{
    Warning out{_logger};
    out << "failed to dispatch " << current.operation << " to ";
    printTarget(out, current);

    if (!exceptionDetails.empty())
    {
        out << ":\n" << exceptionDetails;
    }
}

void
LoggerMiddleware::logDispatchException(const LocalException& ex, const Current& current) const noexcept
{
    ostringstream os;
    os << ex;
    logDispatchException(os.str(), current);
}

void
LoggerMiddleware::printTarget(LoggerOutputBase& out, const Current& current) const noexcept
{
    out << identityToString(current.id, _toStringMode);

    if (!current.facet.empty())
    {
        out << " -f " << escapeString(current.facet, "", _toStringMode);
    }
    out << " over ";

    if (current.con)
    {
        ConnectionInfoPtr connInfo = current.con->getInfo();
        while (connInfo->underlying)
        {
            connInfo = connInfo->underlying;
        }

        if (auto ipConnInfo = dynamic_pointer_cast<IPConnectionInfo>(connInfo))
        {
            out << ipConnInfo->localAddress << ':' << ipConnInfo->localPort << "<->" << ipConnInfo->remoteAddress << ':'
                << ipConnInfo->remotePort;
        }
        else
        {
            // Connection::toString returns a multiline string, so we just use type() here for bt and similar.
            out << current.con->type();
        }
    }
    else
    {
        out << "colloc";
    }
}
