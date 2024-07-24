//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include "Ice/Ice.h"
#include "Test.h"
#include "TestHelper.h"
#include "TestI.h"

#include <chrono>
#include <thread>

using namespace std;
using namespace Test;

namespace
{
    Ice::ConnectionPtr connect(const Ice::ObjectPrx& prx)
    {
        //
        // Establish connection with the given proxy (which might have a timeout
        // set and might sporadically fail on connection establishment if it's
        // too slow). The loop ensures that the connection is established by retrying
        // in case we can a ConnectTimeoutException
        //
        int nRetry = 10;
        while (--nRetry > 0)
        {
            try
            {
                prx->ice_getConnection(); // Establish connection
                break;
            }
            catch (const Ice::ConnectTimeoutException&)
            {
                // Can sporadically occur with slow machines
            }
        }
        return prx->ice_getConnection();
    }
}

void
allTestsWithController(Test::TestHelper* helper, const ControllerPrx& controller)
{
    Ice::CommunicatorPtr communicator = helper->communicator();
    string sref = "timeout:" + helper->getTestEndpoint();

    TimeoutPrx timeout(communicator, sref);

    cout << "testing connect timeout... " << flush;
    {
        //
        // Expect ConnectTimeoutException.
        //
        controller->holdAdapter(-1);
        try
        {
            timeout->op();
            test(false);
        }
        catch (const Ice::ConnectTimeoutException&)
        {
            // Expected.
        }
        controller->resumeAdapter();
        timeout->op(); // Ensure adapter is active.
    }
    {
        //
        // Expect success.
        //
        Ice::InitializationData initData;
        initData.properties = communicator->getProperties()->clone();
        initData.properties->setProperty("Ice.Connection.ConnectTimeout", "-1");
        Ice::CommunicatorHolder ich(initData);

        TimeoutPrx to(ich.communicator(), sref);
        controller->holdAdapter(100);
        try
        {
            to->op();
        }
        catch (const Ice::ConnectTimeoutException&)
        {
            test(false);
        }
    }
    cout << "ok" << endl;

    cout << "testing invocation timeout... " << flush;
    {
        Ice::ConnectionPtr connection = timeout->ice_getConnection();
        TimeoutPrx to = timeout->ice_invocationTimeout(100);
        test(connection == to->ice_getConnection());
        try
        {
            to->sleep(1000);
            test(false);
        }
        catch (const Ice::InvocationTimeoutException&)
        {
        }
        timeout->ice_ping();
        to = timeout->ice_invocationTimeout(1000);
        test(connection == to->ice_getConnection());
        try
        {
            to->sleep(100);
        }
        catch (const Ice::InvocationTimeoutException&)
        {
            test(false);
        }
        test(connection == to->ice_getConnection());
    }
    {
        //
        // Expect InvocationTimeoutException.
        //
        TimeoutPrx to = timeout->ice_invocationTimeout(100);

        auto f = to->sleepAsync(1000);
        try
        {
            f.get();
            test(false);
        }
        catch (const Ice::InvocationTimeoutException&)
        {
        }
        catch (...)
        {
            test(false);
        }
        timeout->ice_ping();
    }
    {
        //
        // Expect success.
        //
        TimeoutPrx to = timeout->ice_invocationTimeout(1000);
        auto f = to->sleepAsync(100);
        try
        {
            f.get();
        }
        catch (...)
        {
            test(false);
        }
    }
    cout << "ok" << endl;

    cout << "testing close timeout... " << flush;
    {
        Ice::ConnectionPtr connection = connect(timeout);
        controller->holdAdapter(-1);
        connection->close(Ice::ConnectionClose::GracefullyWithWait);
        try
        {
            connection->getInfo(); // getInfo() doesn't throw in the closing state.
        }
        catch (const Ice::LocalException&)
        {
            test(false);
        }
        while (true)
        {
            try
            {
                connection->getInfo();
                this_thread::sleep_for(chrono::milliseconds(10));
            }
            catch (const Ice::ConnectionClosedException& ex)
            {
                // Expected.
                test(ex.closedByApplication());
                break;
            }
        }
        controller->resumeAdapter();
        timeout->op(); // Ensure adapter is active.
    }
    cout << "ok" << endl;

    cout << "testing invocation timeouts with collocated calls... " << flush;
    {
        communicator->getProperties()->setProperty("TimeoutCollocated.AdapterId", "timeoutAdapter");

        Ice::ObjectAdapterPtr adapter = communicator->createObjectAdapter("TimeoutCollocated");
        adapter->activate();

        timeout = adapter->addWithUUID<TimeoutPrx>(std::make_shared<TimeoutI>());
        timeout = timeout->ice_invocationTimeout(100);
        try
        {
            timeout->sleep(500);
            test(false);
        }
        catch (const Ice::InvocationTimeoutException&)
        {
        }

        try
        {
            timeout->sleepAsync(500).get();
            test(false);
        }
        catch (const Ice::InvocationTimeoutException&)
        {
        }

        TimeoutPrx batchTimeout = timeout->ice_batchOneway();
        batchTimeout->ice_ping();
        batchTimeout->ice_ping();
        batchTimeout->ice_ping();

        // Keep the server thread pool busy.
        timeout->ice_invocationTimeout(-1)->sleepAsync(500);
        try
        {
            batchTimeout->ice_flushBatchRequestsAsync().get();
            test(false);
        }
        catch (const Ice::InvocationTimeoutException&)
        {
        }

        adapter->destroy();
    }
    cout << "ok" << endl;

    controller->shutdown();
}

void
allTests(Test::TestHelper* helper)
{
    ControllerPrx controller(helper->communicator(), "controller:" + helper->getTestEndpoint(1));

    try
    {
        allTestsWithController(helper, controller);
    }
    catch (const Ice::Exception&)
    {
        // Ensure the adapter is not in the holding state when an unexpected exception occurs to prevent the test
        // from hanging on exit in case a connection which disables timeouts is still opened.
        controller->resumeAdapter();
        throw;
    }
}
