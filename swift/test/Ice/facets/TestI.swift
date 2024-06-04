//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

import Ice

class AI: A {
    func callA(current _: Ice.Current) throws -> String {
        return "A"
    }
}

class BI: B {
    func callA(current _: Ice.Current) throws -> String {
        return "A"
    }

    func callB(current _: Ice.Current) throws -> String {
        return "B"
    }
}

class CI: C {
    func callA(current _: Ice.Current) throws -> String {
        return "A"
    }

    func callC(current _: Ice.Current) throws -> String {
        return "C"
    }
}

class DI: D {
    func callA(current _: Ice.Current) throws -> String {
        return "A"
    }

    func callB(current _: Ice.Current) throws -> String {
        return "B"
    }

    func callC(current _: Ice.Current) throws -> String {
        return "C"
    }

    func callD(current _: Ice.Current) throws -> String {
        return "D"
    }
}

class EI: E {
    func callE(current _: Ice.Current) throws -> String {
        return "E"
    }
}

class FI: F {
    func callE(current _: Ice.Current) throws -> String {
        return "E"
    }

    func callF(current _: Ice.Current) throws -> String {
        return "F"
    }
}

class GI: G {
    var _communicator: Ice.Communicator

    public init(communicator: Ice.Communicator) {
        _communicator = communicator
    }

    func callG(current _: Ice.Current) throws -> String {
        return "G"
    }

    func shutdown(current _: Ice.Current) throws {
        _communicator.shutdown()
    }
}

class HI: H {
    let _communicator: Ice.Communicator

    init(communicator: Ice.Communicator) {
        _communicator = communicator
    }

    func callG(current _: Ice.Current) throws -> String {
        return "G"
    }

    func callH(current _: Ice.Current) throws -> String {
        return "H"
    }

    func shutdown(current _: Ice.Current) throws {
        _communicator.shutdown()
    }
}
