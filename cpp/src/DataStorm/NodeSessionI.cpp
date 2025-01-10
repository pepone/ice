//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include "NodeSessionI.h"
#include "ConnectionManager.h"
#include "Instance.h"
#include "NodeSessionManager.h"
#include "TraceUtil.h"

using namespace std;
using namespace DataStormI;
using namespace DataStormContract;
using namespace Ice;

namespace
{
    // The NodeForwarder class forwards calls to a Node that lacks a public endpoint.
    //
    // This class implements the Slice DataContract::Node interface by forwarding calls to the target Node object
    // using the connection established during the creation of the NodeSession object.
    //
    // The NodeForwarder wraps the node and session proxy parameters passed to the DataContract::Node operations
    // in forwarder proxies, which handle forwarding to the corresponding target objects.
    class NodeForwarder : public Node, public enable_shared_from_this<NodeForwarder>
    {
    public:
        NodeForwarder(
            shared_ptr<NodeSessionManager> nodeSessionManager,
            const shared_ptr<NodeSessionI>& nodeSession,
            NodePrx node)
            : _nodeSessionManager(std::move(nodeSessionManager)),
              _nodeSession(nodeSession),
              _node(std::move(node))
        {
        }

        void initiateCreateSession(optional<NodePrx> publisher, const Current& current) final
        {
            checkNotNull(publisher, __FILE__, __LINE__, current);
            if (auto nodeSession = _nodeSession.lock())
            {
                try
                {
                    optional<SessionPrx> sessionPrx;
                    updateNodeAndSessionProxy(nodeSession, *publisher, sessionPrx, current);
                    // Forward the call to the target Node object, don't need to wait for the result.
                    _node->initiateCreateSessionAsync(publisher, nullptr);
                }
                catch (const CommunicatorDestroyedException&)
                {
                }
            }
        }

        void createSession(
            optional<NodePrx> subscriber,
            optional<SubscriberSessionPrx> subscriberSession,
            bool /* fromRelay */,
            optional<bool> /*subscriberIsHostedOnRelay*/,
            const Current& current) final
        {
            checkNotNull(subscriber, __FILE__, __LINE__, current);
            checkNotNull(subscriberSession, __FILE__, __LINE__, current);

            bool subscriberIsHostedOnRelay =
                (subscriber->ice_getEndpoints().empty() && subscriber->ice_getAdapterId().empty());

            if (auto nodeSession = _nodeSession.lock())
            {
                try
                {
                    updateNodeAndSessionProxy(nodeSession, *subscriber, subscriberSession, current);
                    // Forward the call to the target Node object, don't need to wait for the result.
                    _node->createSessionAsync(subscriber, subscriberSession, true, subscriberIsHostedOnRelay, nullptr);
                }
                catch (const CommunicatorDestroyedException&)
                {
                }
            }
        }

        void confirmCreateSession(
            optional<NodePrx> publisher,
            optional<PublisherSessionPrx> publisherSession,
            const Current& current) final
        {
            checkNotNull(publisher, __FILE__, __LINE__, current);
            checkNotNull(publisherSession, __FILE__, __LINE__, current);

            if (auto nodeSession = _nodeSession.lock())
            {
                // Checks whether there is an active NodeSession for the publisher matching the current connection.
                // If there is a match, forward the call to the target node.
                auto publisherNodeSession = _nodeSessionManager->getSession(publisher->ice_getIdentity());
                if (publisherNodeSession && publisherNodeSession->getConnection() == current.con)
                {
                    try
                    {
                        updateNodeAndSessionProxy(nodeSession, *publisher, publisherSession, current);
                        // Forward the call to the target Node object, don't need to wait for the result.
                        _node->confirmCreateSessionAsync(publisher, publisherSession, nullptr);
                    }
                    catch (const CommunicatorDestroyedException&)
                    {
                    }
                }
                else
                {
                    publisherSession->ice_fixed(current.con)->disconnectedAsync(nullptr);
                }
            }
        }

    private:
        // This helper method is used to replace the Node and Session proxies with forwarders when the calling Node
        // doesn't have a public endpoint.
        //
        // The subscriber or publisher session is added to the NodeSession. The NodeSession uses this proxy to inform
        // the publisher or subscriber of the disconnection when the NodeSession connection is closed.
        template<typename T>
        void updateNodeAndSessionProxy(
            const shared_ptr<NodeSessionI>& nodeSession,
            NodePrx& node,
            optional<T>& session,
            const Current& current)
        {
            if (node->ice_getEndpoints().empty() && node->ice_getAdapterId().empty())
            {
                shared_ptr<NodeSessionI> peerNodeSession = _nodeSessionManager->createOrGet(node, current.con, false);
                node = peerNodeSession->getPublicNode();
                if (session)
                {
                    nodeSession->addSession(session->ice_fixed(current.con));
                    session = peerNodeSession->forwarder(*session);
                }
            }
            else if (session)
            {
                nodeSession->addSession(*session);
            }
        }

        const shared_ptr<NodeSessionManager> _nodeSessionManager;
        const weak_ptr<NodeSessionI> _nodeSession;
        const NodePrx _node;
    };
}

NodeSessionI::NodeSessionI(
    shared_ptr<Instance> instance,
    const shared_ptr<NodeSessionManager>& nodeSessionManager,
    NodePrx node,
    ConnectionPtr connection,
    bool forwardAnnouncements)
    : _instance(std::move(instance)),
      _nodeSessionManager(nodeSessionManager),
      _node(std::move(node)),
      _connection(std::move(connection))
{
    if (forwardAnnouncements)
    {
        _lookup = _connection->createProxy<LookupPrx>(Identity{.name = "Lookup", .category = "DataStorm"});
    }
}

void
NodeSessionI::init()
{
    // When the target node doesn't have a public endpoint, we create a NodeForwarder object to forward calls to the
    // target and assign it to the publicNode member, otherwise the publicNode member is set to the target node.
    if (_node->ice_getEndpoints().empty() && _node->ice_getAdapterId().empty())
    {
        _publicNode = _instance->getObjectAdapter()->add<NodePrx>(
            make_shared<NodeForwarder>(
                _instance->getNodeSessionManager(),
                shared_from_this(),
                _node->ice_fixed(_connection)),
            _node->ice_getIdentity());
    }
    else
    {
        _publicNode = _node;
    }

    if (_instance->getTraceLevels()->session > 0)
    {
        Trace out(_instance->getTraceLevels()->logger, _instance->getTraceLevels()->sessionCat);
        out << "created node session (peer = '" << _publicNode << "'):\n" << _connection->toString();
    }
}

void
NodeSessionI::destroy()
{
    lock_guard<mutex> lock(_mutex);
    try
    {
        if (_publicNode != _node)
        {
            // Remove the NodeForwarder object from the object adapter.
            _instance->getObjectAdapter()->remove(_publicNode->ice_getIdentity());
        }

        if (auto nodeSessionManager = _nodeSessionManager.lock())
        {
            // Notify sessions of the disconnection, don't need to wait for the result.
            for (const auto& [_, session] : _sessions)
            {
                session->disconnectedAsync(nullptr);
            }
        }
    }
    catch (const ObjectAdapterDestroyedException&)
    {
    }
    catch (const CommunicatorDestroyedException&)
    {
    }

    if (_instance->getTraceLevels()->session > 0)
    {
        Trace out(_instance->getTraceLevels()->logger, _instance->getTraceLevels()->sessionCat);
        out << "destroyed node session (peer = '" << _publicNode << "')";
    }
}

void
NodeSessionI::addSession(SessionPrx session)
{
    lock_guard<mutex> lock(_mutex);
    Identity id = session->ice_getIdentity();
    _sessions.insert_or_assign(std::move(id), std::move(session));
}
