//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include <Glacier2/ServerBlobject.h>

using namespace std;
using namespace Ice;
using namespace Glacier2;

Glacier2::ServerBlobject::ServerBlobject(shared_ptr<Instance> instance, shared_ptr<Connection> connection)
    : Glacier2::Blobject(std::move(instance), std::move(connection), Ice::Context())
{
}

void
Glacier2::ServerBlobject::ice_invokeAsync(
    pair<const byte*, const byte*> inParams,
    function<void(bool, const pair<const byte*, const byte*>&)> response,
    function<void(exception_ptr)> error,
    const Current& current)
{
    auto proxy = _reverseConnection->createProxy(current.id);
    invoke(proxy, inParams, std::move(response), std::move(error), current);
}
