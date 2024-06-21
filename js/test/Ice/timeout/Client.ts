//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

import { Ice } from "ice";
import { Test } from "./Test.js";
import { TestHelper } from "../../Common/TestHelper.js";

const test = TestHelper.test;

export class Client extends TestHelper {
    async allTests() {
        const controller = Test.ControllerPrx.uncheckedCast(
            this.communicator().stringToProxy("controller:" + this.getTestEndpoint(1)),
        );
        test(controller !== null);
        try {
            await this.allTestsWithController(controller);
        } catch (ex) {
            // Ensure the adapter is not in the holding state when an unexpected exception occurs to prevent the test
            // from hanging on exit in case a connection which disables timeouts is still opened.
            controller.resumeAdapter();
            throw ex;
        }
    }

    async allTestsWithController(controller: Test.ControllerPrx) {
        const communicator = this.communicator();
        const out = this.getWriter();

        const timeout = Test.TimeoutPrx.uncheckedCast(communicator.stringToProxy(`timeout: ${this.getTestEndpoint()}`));

        out.write("testing connect timeout... ");
        {
            await controller.holdAdapter(-1);
            try {
                await timeout.op(); // Expect ConnectTimeoutException.
                test(false);
            } catch (ex) {
                test(ex instanceof Ice.ConnectTimeoutException, ex);
            }
            controller.resumeAdapter();
            await timeout.op(); // Ensure adapter is active.
        }

        {
            var properties = communicator.getProperties().clone();
            properties.setProperty("Ice.Connection.ConnectTimeout", "-1");
            const [communicator2, _] = this.initialize(properties);

            const to = Test.TimeoutPrx.uncheckedCast(communicator2.stringToProxy(timeout.toString()));
            controller.holdAdapter(100);
            try {
                await to.op(); // Expect success.
            } catch (ex) {
                test(false, ex);
            } finally {
                await controller.resumeAdapter();
                await communicator2.destroy();
            }
        }
        out.writeLine("ok");

        out.write("testing invocation timeout... ");
        {
            const connection = await timeout.ice_getConnection();
            let to = timeout.ice_invocationTimeout(100);
            test(connection == (await to.ice_getConnection()));

            try {
                await to.sleep(1000);
                test(false);
            } catch (ex) {
                test(ex instanceof Ice.InvocationTimeoutException, ex);
            }
            await timeout.ice_ping();
            to = timeout.ice_invocationTimeout(1000);
            test(connection === (await timeout.ice_getConnection()));

            try {
                await to.sleep(100);
            } catch (ex) {
                test(false);
            }
        }
        out.writeLine("ok");

        out.write("testing close timeout... ");
        {
            const connection = await timeout.ice_getConnection();
            await controller.holdAdapter(-1);
            await connection.close(Ice.ConnectionClose.GracefullyWithWait);

            try {
                connection.getInfo(); // getInfo() doesn't throw in the closing state
                while (true) {
                    try {
                        connection.getInfo();
                        await Ice.Promise.delay(10);
                    } catch (ex) {
                        test(ex instanceof Ice.ConnectionManuallyClosedException, ex); // Expected
                        test(ex.graceful);
                        break;
                    }
                }
            } catch (ex) {
                test(false, ex);
            } finally {
                await controller.resumeAdapter();
            }
            await timeout.op();
        }
        controller.shutdown();
        out.writeLine("ok");
    }

    async run(args: string[]) {
        let communicator: Ice.Communicator | null = null;
        try {
            const [properties] = this.createTestProperties(args);
            //
            // For this test, we want to disable retries.
            //
            properties.setProperty("Ice.RetryIntervals", "-1");

            properties.setProperty("Ice.Connection.ConnectTimeout", "1");
            properties.setProperty("Ice.Connection.CloseTimeout", "1");

            //
            // We don't want connection warnings because of the timeout
            //
            properties.setProperty("Ice.Warn.Connections", "0");
            properties.setProperty("Ice.PrintStackTraces", "1");

            [communicator] = this.initialize(properties);
            await this.allTests();
        } finally {
            if (communicator) {
                await communicator.destroy();
            }
        }
    }
}
