//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include "../Ice/DisableWarnings.h"

#include "../Ice/SSL/RFC2253.h"
#include "../Ice/SSL/SSLUtil.h"
#include "Database.h"
#include "FileCache.h"
#include "Ice/Ice.h"
#include "InternalRegistryI.h"
#include "NodeSessionI.h"
#include "ReapThread.h"
#include "RegistryI.h"
#include "ReplicaSessionI.h"
#include "ReplicaSessionManager.h"
#include "Topics.h"
#include "WellKnownObjectsManager.h"

using namespace std;
using namespace IceGrid;

InternalRegistryI::InternalRegistryI(
    const shared_ptr<RegistryI>& registry,
    const shared_ptr<Database>& database,
    const shared_ptr<ReapThread>& reaper,
    const shared_ptr<WellKnownObjectsManager>& wellKnownObjects,
    ReplicaSessionManager& session)
    : _registry(registry),
      _database(database),
      _reaper(reaper),
      _wellKnownObjects(wellKnownObjects),
      _fileCache(make_shared<FileCache>(database->getCommunicator())),
      _session(session)
{
    auto properties = database->getCommunicator()->getProperties();
    // TODO: temporary. For now, synchronized with the default idle timeout.
    _nodeSessionTimeout = chrono::seconds(60);
    _replicaSessionTimeout = chrono::seconds(60);
}

optional<NodeSessionPrx>
InternalRegistryI::registerNode(
    shared_ptr<InternalNodeInfo> info,
    optional<NodePrx> node,
    LoadInfo load,
    const Ice::Current& current)
{
    const auto traceLevels = _database->getTraceLevels();
    const auto logger = traceLevels->logger;

    Ice::checkNotNull(node, __FILE__, __LINE__, current);

    if (!info)
    {
        std::ostringstream os;
        os << "null node info passed to " << current.operation << " on object "
           << current.adapter->getCommunicator()->identityToString(current.id);
        throw Ice::MarshalException{__FILE__, __LINE__, os.str()};
    }

    try
    {
        auto session = NodeSessionI::create(_database, std::move(*node), info, _nodeSessionTimeout, load);
        _reaper->add(make_shared<SessionReapable<NodeSessionI>>(logger, session), _nodeSessionTimeout);
        return session->getProxy();
    }
    catch (const Ice::ObjectAdapterDeactivatedException&)
    {
        throw Ice::ObjectNotExistException(__FILE__, __LINE__);
    }
}

optional<ReplicaSessionPrx>
InternalRegistryI::registerReplica(
    shared_ptr<InternalReplicaInfo> info,
    optional<InternalRegistryPrx> prx,
    const Ice::Current& current)
{
    const auto traceLevels = _database->getTraceLevels();
    const auto logger = traceLevels->logger;

    Ice::checkNotNull(prx, __FILE__, __LINE__, current);

    if (!info)
    {
        std::ostringstream os;
        os << "null replica info passed to " << current.operation << " on object "
           << current.adapter->getCommunicator()->identityToString(current.id);
        throw Ice::MarshalException{__FILE__, __LINE__, os.str()};
    }

    try
    {
        auto s = ReplicaSessionI::create(_database, _wellKnownObjects, info, std::move(*prx), _replicaSessionTimeout);
        _reaper->add(make_shared<SessionReapable<ReplicaSessionI>>(logger, s), _replicaSessionTimeout);
        return s->getProxy();
    }
    catch (const Ice::ObjectAdapterDeactivatedException&)
    {
        throw Ice::ObjectNotExistException(__FILE__, __LINE__);
    }
}

void
InternalRegistryI::registerWithReplica(optional<InternalRegistryPrx> replica, const Ice::Current& current)
{
    Ice::checkNotNull(replica, __FILE__, __LINE__, current);
    _session.create(std::move(*replica));
}

NodePrxSeq
InternalRegistryI::getNodes(const Ice::Current&) const
{
    NodePrxSeq nodes;
    for (const auto& proxy : _database->getInternalObjectsByType(string{Node::ice_staticId()}))
    {
        assert(proxy);
        nodes.push_back(Ice::uncheckedCast<NodePrx>(*proxy));
    }
    return nodes;
}

InternalRegistryPrxSeq
InternalRegistryI::getReplicas(const Ice::Current&) const
{
    InternalRegistryPrxSeq replicas;
    for (const auto& proxy : _database->getObjectsByType(string{InternalRegistry::ice_staticId()}))
    {
        assert(proxy);
        replicas.push_back(Ice::uncheckedCast<InternalRegistryPrx>(*proxy));
    }
    return replicas;
}

ApplicationInfoSeq
InternalRegistryI::getApplications(int64_t& serial, const Ice::Current&) const
{
    return _database->getApplications(serial);
}

AdapterInfoSeq
InternalRegistryI::getAdapters(int64_t& serial, const Ice::Current&) const
{
    return _database->getAdapters(serial);
}

ObjectInfoSeq
InternalRegistryI::getObjects(int64_t& serial, const Ice::Current&) const
{
    return _database->getObjects(serial);
}

void
InternalRegistryI::shutdown(const Ice::Current& /*current*/) const
{
    _registry->shutdown();
}

int64_t
InternalRegistryI::getOffsetFromEnd(string filename, int count, const Ice::Current&) const
{
    return _fileCache->getOffsetFromEnd(getFilePath(filename), count);
}

bool
InternalRegistryI::read(
    string filename,
    int64_t pos,
    int size,
    int64_t& newPos,
    Ice::StringSeq& lines,
    const Ice::Current&) const
{
    return _fileCache->read(getFilePath(filename), pos, size, newPos, lines);
}

string
InternalRegistryI::getFilePath(const string& filename) const
{
    string file;
    if (filename == "stderr")
    {
        file = _database->getCommunicator()->getProperties()->getProperty("Ice.StdErr");
        if (file.empty())
        {
            throw FileNotAvailableException("Ice.StdErr configuration property is not set");
        }
    }
    else if (filename == "stdout")
    {
        file = _database->getCommunicator()->getProperties()->getProperty("Ice.StdOut");
        if (file.empty())
        {
            throw FileNotAvailableException("Ice.StdOut configuration property is not set");
        }
    }
    else
    {
        throw FileNotAvailableException("unknown file");
    }
    return file;
}
