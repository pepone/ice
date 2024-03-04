//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include <Glacier2/PermissionsVerifier.h>
#include <Glacier2/NullPermissionsVerifier.h>
#include <Ice/Ice.h>

using namespace Ice;
using namespace std;

namespace
{

class NullPermissionsVerifier final : public Glacier2::PermissionsVerifier
{
public:

    bool checkPermissions(string, string, string&, const Current&) const final
    {
        return true;
    }
};

class NullSSLPermissionsVerifier : public Glacier2::SSLPermissionsVerifier
{
public:

    bool authorize(Glacier2::SSLInfo, string&, const Ice::Current&) const final
    {
        return true;
    }
};

Ice::ObjectAdapterPtr createObjectAdapter(
    const Ice::CommunicatorPtr& communicator,
    const Ice::ObjectAdapterPtr& adapter)
{
    if (adapter == nullptr)
    {
        Ice::ObjectAdapterPtr newAdapter = communicator->createObjectAdapter("");
        newAdapter->activate();
        return newAdapter;
    }
    else
    {
        return adapter;
    }
}

}

namespace Glacier2Internal
{

void
setupNullPermissionsVerifier(
    const CommunicatorPtr& communicator,
    const string& category,
    const vector<string>& permissionsVerifierPropertyNames)
{
    const Ice::Identity nullPermissionsVerifierId { "NullPermissionsVerifier", category };
    const Ice::Identity nullSSLPermissionsVerifierId { "NullSSLPermissionsVerifier",  category };

    Ice::PropertiesPtr properties = communicator->getProperties();
    ObjectAdapterPtr adapter;
    for (const auto& propertyName : permissionsVerifierPropertyNames)
    {
        string val = properties->getProperty(propertyName);
        if (!val.empty())
        {
            try
            {
                ObjectPrx prx(communicator, val);
                if (prx->ice_getIdentity() == nullPermissionsVerifierId)
                {
                    adapter = createObjectAdapter(communicator, adapter);
                    adapter->add(make_shared<NullPermissionsVerifier>(), nullPermissionsVerifierId);
                }
                else if (prx->ice_getIdentity() == nullSSLPermissionsVerifierId)
                {
                    adapter = createObjectAdapter(communicator, adapter);
                    adapter->add(make_shared<NullSSLPermissionsVerifier>(), nullSSLPermissionsVerifierId);
                }
            }
            catch(const ProxyParseException&)
            {
                // check if it's actually a stringified identity
                // (with typically missing " " because the category contains a space)

                if(val == communicator->identityToString(nullPermissionsVerifierId))
                {
                    adapter = createObjectAdapter(communicator, adapter);
                    ObjectPrx prx = adapter->add(make_shared<NullPermissionsVerifier>(), nullPermissionsVerifierId);
                    properties->setProperty(propertyName, prx->ice_toString());
                }
                else if(val == communicator->identityToString(nullSSLPermissionsVerifierId))
                {
                    adapter = createObjectAdapter(communicator, adapter);
                    ObjectPrx prx = adapter->add(
                        make_shared<NullSSLPermissionsVerifier>(),
                        nullSSLPermissionsVerifierId);
                    properties->setProperty(propertyName, prx->ice_toString());
                }
                // Otherwise let the service report this incorrectly formatted proxy
            }
        }
    }
}

}
