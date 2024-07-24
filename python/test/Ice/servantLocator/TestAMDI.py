#
# Copyright (c) ZeroC, Inc. All rights reserved.
#

import Ice
import Test


def test(b):
    if not b:
        raise RuntimeError("test assertion failed")


class TestI(Test.TestIntf):
    def requestFailedException(self, current):
        return None

    def unknownUserException(self, current):
        return None

    def unknownLocalException(self, current):
        return None

    def unknownException(self, current):
        return None

    def localException(self, current):
        return None

    def userException(self, current):
        return None

    def pythonException(self, current):
        return None

    def unknownExceptionWithServantException(self, current):
        f = Ice.Future()
        f.set_exception(Ice.ObjectNotExistException())
        return f

    def impossibleException(self, throw, current):
        f = Ice.Future()
        if throw:
            f.set_exception(Test.TestImpossibleException())
        else:
            #
            # Return a value so we can be sure that the stream position
            # is reset correctly if finished() throws.
            #
            f.set_result("Hello")
        return f

    def intfUserException(self, throw, current):
        f = Ice.Future()
        if throw:
            f.set_exception(Test.TestIntfUserException())
        else:
            #
            # Return a value so we can be sure that the stream position
            # is reset correctly if finished() throws.
            #
            f.set_result("Hello")
        return f

    def asyncResponse(self, current):
        #
        # We can't do this with futures.
        #
        # return Ice.Future.completed(None)
        raise Ice.ObjectNotExistException()

    def asyncException(self, current):
        #
        # We can't do this with futures.
        #
        # f = Ice.Future()
        # f.set_exception(Test.TestIntfUserException())
        # return f
        raise Ice.ObjectNotExistException()

    def shutdown(self, current):
        current.adapter.deactivate()


class Cookie:
    def message(self):
        return "blahblah"


class ServantLocatorI(Ice.ServantLocator):
    def __init__(self, category):
        self._deactivated = False
        self._category = category
        self._requestId = -1

    def __del__(self):
        test(self._deactivated)

    def locate(self, current):
        test(not self._deactivated)

        test(current.id.category == self._category or self._category == "")

        if current.id.name == "unknown":
            return None

        if current.id.name == "invalidReturnValue":
            return (45, 12)

        if current.id.name == "invalidReturnType":
            return "invalid"

        test(current.id.name == "locate" or current.id.name == "finished")
        if current.id.name == "locate":
            self.exception(current)

        #
        # Ensure locate() is only called once per request.
        #
        test(self._requestId == -1)
        self._requestId = current.requestId

        return (TestI(), Cookie())

    def finished(self, current, servant, cookie):
        test(not self._deactivated)

        #
        # Ensure finished() is only called once per request.
        #
        test(self._requestId == current.requestId)
        self._requestId = -1

        test(current.id.category == self._category or self._category == "")
        test(current.id.name == "locate" or current.id.name == "finished")

        if current.id.name == "finished":
            self.exception(current)

        test(isinstance(cookie, Cookie))
        test(cookie.message() == "blahblah")

    def deactivate(self, category):
        test(not self._deactivated)

        self._deactivated = True

    def exception(self, current):
        if current.operation == "ice_ids":
            raise Test.TestIntfUserException()
        elif current.operation == "requestFailedException":
            raise Ice.ObjectNotExistException()
        elif current.operation == "unknownUserException":
            raise Ice.UnknownUserException("reason")
        elif current.operation == "unknownLocalException":
            raise Ice.UnknownLocalException("reason")
        elif current.operation == "unknownException":
            raise Ice.UnknownException("reason")
        elif current.operation == "userException":
            raise Test.TestIntfUserException()
        elif current.operation == "localException":
            raise Ice.SocketException(0)
        elif current.operation == "pythonException":
            raise RuntimeError("message")
        elif current.operation == "unknownExceptionWithServantException":
            raise Ice.UnknownException("reason")
        elif current.operation == "impossibleException":
            raise Test.TestIntfUserException()  # Yes, it really is meant to be TestIntfUserException.
        elif current.operation == "intfUserException":
            raise Test.TestImpossibleException()  # Yes, it really is meant to be TestImpossibleException.
        elif current.operation == "asyncResponse":
            raise Test.TestImpossibleException()
        elif current.operation == "asyncException":
            raise Test.TestImpossibleException()
