# Copyright (c) ZeroC, Inc.

import sys
import Ice
import Test


def test(b):
    if not b:
        raise RuntimeError("test assertion failed")


def testExceptions(obj):
    try:
        obj.requestFailedException()
        test(False)
    except Ice.ObjectNotExistException as ex:
        test(ex.id == obj.ice_getIdentity())
        test(ex.facet == obj.ice_getFacet())
        test(ex.operation == "requestFailedException")
    except Exception:
        test(False)

    try:
        obj.unknownUserException()
        test(False)
    except Ice.UnknownUserException as ex:
        test("reason" == str(ex))
    except Exception:
        test(False)

    try:
        obj.unknownLocalException()
        test(False)
    except Ice.UnknownLocalException as ex:
        test("reason" == str(ex))
    except Exception:
        test(False)

    try:
        obj.unknownException()
        test(False)
    except Ice.UnknownException as ex:
        test("reason" == str(ex))
        pass

    try:
        obj.userException()
        test(False)
    except Ice.UnknownUserException as ex:
        test("::Test::TestIntfUserException" in str(ex))
    except Ice.OperationNotExistException:
        pass
    except AttributeError:
        pass
    except Exception:
        test(False)

    try:
        obj.localException()
        test(False)
    except Ice.UnknownLocalException as ex:
        test("Ice.SocketException" in str(ex) or "Ice::SocketException" in str(ex))
    except Exception:
        test(False)

    try:
        obj.pythonException()
        test(False)
    except Ice.UnknownException as ex:
        test("RuntimeError: message" in str(ex))
    except Ice.OperationNotExistException:
        pass
    except AttributeError:
        pass
    except Exception:
        test(False)

    try:
        obj.unknownExceptionWithServantException()
        test(False)
    except Ice.UnknownException as ex:
        test("reason" in str(ex))
    except Exception:
        test(False)

    try:
        obj.impossibleException(False)
        test(False)
    except Ice.UnknownUserException:
        # Operation doesn't throw, but locate() and finished() throw TestIntfUserException.
        pass
    except Exception:
        test(False)

    try:
        obj.impossibleException(True)
        test(False)
    except Ice.UnknownUserException:
        # Operation doesn't throw, but locate() and finished() throw TestIntfUserException.
        pass
    except Exception:
        test(False)

    try:
        obj.intfUserException(False)
        test(False)
    except Test.TestImpossibleException:
        # Operation doesn't throw, but locate() and finished() throw TestImpossibleException.
        pass
    except Exception:
        test(False)

    try:
        obj.intfUserException(True)
        test(False)
    except Test.TestImpossibleException:
        # Operation throws TestIntfUserException, but locate() and finished() throw TestImpossibleException.
        pass
    except Exception:
        test(False)


def allTests(helper, communicator):
    sys.stdout.write("testing stringToProxy... ")
    sys.stdout.flush()
    base = communicator.stringToProxy("asm:{0}".format(helper.getTestEndpoint()))
    test(base)
    print("ok")

    sys.stdout.write("testing checked cast... ")
    sys.stdout.flush()
    obj = Test.TestIntfPrx.checkedCast(base)
    test(obj)
    test(obj == base)
    print("ok")

    sys.stdout.write("testing ice_ids... ")
    sys.stdout.flush()
    try:
        obj = communicator.stringToProxy("category/locate:{0}".format(helper.getTestEndpoint()))
        obj.ice_ids()
        test(False)
    except Ice.UnknownUserException as ex:
        test("::Test::TestIntfUserException" in str(ex))
    except Exception:
        test(False)

    try:
        obj = communicator.stringToProxy("category/finished:{0}".format(helper.getTestEndpoint()))
        obj.ice_ids()
        test(False)
    except Ice.UnknownUserException as ex:
        test("::Test::TestIntfUserException" in str(ex))
    except Exception:
        test(False)
    print("ok")

    sys.stdout.write("testing servant locator... ")
    sys.stdout.flush()
    base = communicator.stringToProxy("category/locate:{0}".format(helper.getTestEndpoint()))
    obj = Test.TestIntfPrx.checkedCast(base)
    try:
        Test.TestIntfPrx.checkedCast(
            communicator.stringToProxy("category/unknown:{0}".format(helper.getTestEndpoint()))
        )
    except Ice.ObjectNotExistException:
        pass
    print("ok")

    sys.stdout.write("testing default servant locator... ")
    sys.stdout.flush()
    base = communicator.stringToProxy("anothercat/locate:{0}".format(helper.getTestEndpoint()))
    obj = Test.TestIntfPrx.checkedCast(base)
    base = communicator.stringToProxy("locate:{0}".format(helper.getTestEndpoint()))
    obj = Test.TestIntfPrx.checkedCast(base)
    try:
        Test.TestIntfPrx.checkedCast(
            communicator.stringToProxy("anothercat/unknown:{0}".format(helper.getTestEndpoint()))
        )
    except Ice.ObjectNotExistException:
        pass
    try:
        Test.TestIntfPrx.checkedCast(communicator.stringToProxy("unknown:{0}".format(helper.getTestEndpoint())))
    except Ice.ObjectNotExistException:
        pass
    print("ok")

    sys.stdout.write("testing locate exceptions... ")
    sys.stdout.flush()
    base = communicator.stringToProxy("category/locate:{0}".format(helper.getTestEndpoint()))
    obj = Test.TestIntfPrx.checkedCast(base)
    testExceptions(obj)
    print("ok")

    sys.stdout.write("testing finished exceptions... ")
    sys.stdout.flush()
    base = communicator.stringToProxy("category/finished:{0}".format(helper.getTestEndpoint()))
    obj = Test.TestIntfPrx.checkedCast(base)
    testExceptions(obj)
    print("ok")

    sys.stdout.write("testing servant locator removal... ")
    sys.stdout.flush()
    base = communicator.stringToProxy("test/activation:{0}".format(helper.getTestEndpoint()))
    activation = Test.TestActivationPrx.checkedCast(base)
    activation.activateServantLocator(False)
    try:
        obj.ice_ping()
        test(False)
    except Ice.ObjectNotExistException:
        pass
    print("ok")

    sys.stdout.write("testing servant locator addition... ")
    sys.stdout.flush()
    activation.activateServantLocator(True)
    try:
        obj.ice_ping()
    except Exception:
        test(False)
    print("ok")

    sys.stdout.write("testing invalid locate return values ... ")
    sys.stdout.flush()
    try:
        communicator.stringToProxy("invalidReturnValue:{0}".format(helper.getTestEndpoint())).ice_ping()
    except Ice.ObjectNotExistException:
        pass
    try:
        communicator.stringToProxy("invalidReturnType:{0}".format(helper.getTestEndpoint())).ice_ping()
    except Ice.ObjectNotExistException:
        pass
    print("ok")

    return obj
