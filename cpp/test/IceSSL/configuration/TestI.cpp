//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include "TestI.h"
#include "Ice/Ice.h"
#include "TestHelper.h"

using namespace std;
using namespace Ice;

ServerI::ServerI(const CommunicatorPtr& communicator) : _communicator(communicator) {}

void
ServerI::noCert(const Ice::Current& c)
{
    try
    {
        Ice::SSL::ConnectionInfoPtr info = dynamic_pointer_cast<Ice::SSL::ConnectionInfo>(c.con->getInfo());
        test(!info->peerCertificate);
    }
    catch (const Ice::LocalException& ex)
    {
        cerr << ex << endl;
        test(false);
    }
}

void
ServerI::checkCert(string, string, const Ice::Current&)
{
    // TODO
    /*
    try
    {
        Ice::SSL::ConnectionInfoPtr info = dynamic_pointer_cast<Ice::SSL::ConnectionInfo>(c.con->getInfo());

        test(info->certs.size() == 2);
        test(info->certs[0]->getSubjectDN() == Ice::SSL::DistinguishedName(subjectDN));
        test(info->certs[0]->getIssuerDN() == Ice::SSL::DistinguishedName(issuerDN));
    }
    catch (const Ice::LocalException&)
    {
        test(false);
    }*/
}

void
ServerI::destroy()
{
    _communicator->destroy();
}

ServerFactoryI::ServerFactoryI(const string& defaultDir) : _defaultDir(defaultDir) {}

optional<Test::ServerPrx>
ServerFactoryI::createServer(Test::Properties props, const Current&)
{
    InitializationData initData;
    initData.properties = createProperties();
    for (Test::Properties::const_iterator p = props.begin(); p != props.end(); ++p)
    {
        initData.properties->setProperty(p->first, p->second);
    }
    initData.properties->setProperty("IceSSL.DefaultDir", _defaultDir);

    CommunicatorPtr communicator = initialize(initData);
    ObjectAdapterPtr adapter = communicator->createObjectAdapterWithEndpoints("ServerAdapter", "ssl");
    ServerIPtr server = make_shared<ServerI>(communicator);
    auto obj = adapter->addWithUUID<Test::ServerPrx>(server);
    _servers[obj->ice_getIdentity()] = server;
    adapter->activate();

    return obj;
}

void
ServerFactoryI::destroyServer(optional<Test::ServerPrx> srv, const Ice::Current&)
{
    map<Identity, ServerIPtr>::iterator p = _servers.find(srv->ice_getIdentity());
    if (p != _servers.end())
    {
        p->second->destroy();
        _servers.erase(p);
    }
}

void
ServerFactoryI::shutdown(const Ice::Current& current)
{
    test(_servers.empty());
    current.adapter->getCommunicator()->shutdown();
}
