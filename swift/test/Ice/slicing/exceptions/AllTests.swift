// Copyright (c) ZeroC, Inc.

import Ice
import PromiseKit
import TestCommon

public func allTests(_ helper: TestHelper) throws -> TestIntfPrx {
    func test(_ value: Bool, file: String = #file, line: Int = #line) throws {
        try helper.test(value, file: file, line: line)
    }

    let output = helper.getWriter()
    let communicator = helper.communicator()

    output.write("testing stringToProxy... ")
    let ref = "Test:\(helper.getTestEndpoint(num: 0)) -t 2000"
    let base = try communicator.stringToProxy(ref)!
    output.writeLine("ok")

    output.write("testing checked cast... ")
    let testPrx = try checkedCast(prx: base, type: TestIntfPrx.self)!
    try test(testPrx == base)
    output.writeLine("ok")

    output.write("base... ")
    do {
        try testPrx.baseAsBase()
        try test(false)
    } catch let b as Base {
        try test(b.b == "Base.b")
    }
    output.writeLine("ok")

    output.write("base (AMI)... ")
    try Promise<Void> { seal in
        firstly {
            testPrx.baseAsBaseAsync()
        }.done {
            try test(false)
        }.catch { e in
            do {
                if let b = e as? Base {
                    try test(b.b == "Base.b")
                } else {
                    try test(false)
                }
                seal.fulfill(())
            } catch {
                seal.reject(error)
            }
        }
    }.wait()
    output.writeLine("ok")

    output.write("slicing of unknown derived... ")
    do {
        try testPrx.unknownDerivedAsBase()
        try test(false)
    } catch let b as Base {
        try test(b.b == "UnknownDerived.b")
    }
    output.writeLine("ok")

    output.write("slicing of unknown derived (AMI)... ")
    try Promise<Void> { seal in
        firstly {
            testPrx.unknownDerivedAsBaseAsync()
        }.done {
            try test(false)
        }.catch { e in
            do {
                if let b = e as? Base {
                    try test(b.b == "UnknownDerived.b")
                } else {
                    try test(false)
                }
                seal.fulfill(())
            } catch {
                seal.reject(error)
            }
        }
    }.wait()
    output.writeLine("ok")

    output.write("non-slicing of known derived as base... ")
    do {
        try testPrx.knownDerivedAsBase()
        try test(false)
    } catch let k as KnownDerived {
        try test(k.b == "KnownDerived.b")
        try test(k.kd == "KnownDerived.kd")
    }
    output.writeLine("ok")

    output.write("non-slicing of known derived as base (AMI)... ")
    try Promise<Void> { seal in
        firstly {
            testPrx.knownDerivedAsBaseAsync()
        }.done {
            try test(false)
        }.catch { e in
            do {
                if let k = e as? KnownDerived {
                    try test(k.b == "KnownDerived.b")
                    try test(k.kd == "KnownDerived.kd")
                } else {
                    try test(false)
                }
                seal.fulfill(())
            } catch {
                seal.reject(error)
            }
        }
    }.wait()
    output.writeLine("ok")

    output.write("non-slicing of known derived as derived... ")
    do {
        try testPrx.knownDerivedAsKnownDerived()
        try test(false)
    } catch let k as KnownDerived {
        try test(k.b == "KnownDerived.b")
        try test(k.kd == "KnownDerived.kd")
    }
    output.writeLine("ok")

    output.write("non-slicing of known derived as derived (AMI)... ")
    try Promise<Void> { seal in
        firstly {
            testPrx.knownDerivedAsKnownDerivedAsync()
        }.done {
            try test(false)
        }.catch { e in
            do {
                if let k = e as? KnownDerived {
                    try test(k.b == "KnownDerived.b")
                    try test(k.kd == "KnownDerived.kd")
                } else {
                    try test(false)
                }
                seal.fulfill(())
            } catch {
                seal.reject(error)
            }
        }
    }.wait()
    output.writeLine("ok")

    output.write("slicing of unknown intermediate as base... ")
    do {
        try testPrx.unknownIntermediateAsBase()
        try test(false)
    } catch let b as Base {
        try test(b.b == "UnknownIntermediate.b")
    }
    output.writeLine("ok")

    output.write("slicing of unknown intermediate as base (AMI)... ")
    try Promise<Void> { seal in
        firstly {
            testPrx.unknownIntermediateAsBaseAsync()
        }.done {
            try test(false)
        }.catch { e in
            do {
                if let b = e as? Base {
                    try test(b.b == "UnknownIntermediate.b")
                } else {
                    try test(false)
                }
                seal.fulfill(())
            } catch {
                seal.reject(error)
            }
        }
    }.wait()
    output.writeLine("ok")

    output.write("slicing of known intermediate as base... ")
    do {
        try testPrx.knownIntermediateAsBase()
        try test(false)
    } catch let ki as KnownIntermediate {
        try test(ki.b == "KnownIntermediate.b")
        try test(ki.ki == "KnownIntermediate.ki")
    }
    output.writeLine("ok")

    output.write("slicing of known intermediate as base (AMI)... ")
    try Promise<Void> { seal in
        firstly {
            testPrx.knownIntermediateAsBaseAsync()
        }.done {
            try test(false)
        }.catch { e in
            do {
                if let ki = e as? KnownIntermediate {
                    try test(ki.b == "KnownIntermediate.b")
                    try test(ki.ki == "KnownIntermediate.ki")
                } else {
                    try test(false)
                }
                seal.fulfill(())
            } catch {
                seal.reject(error)
            }
        }
    }.wait()
    output.writeLine("ok")

    output.write("slicing of known most derived as base... ")
    do {
        try testPrx.knownMostDerivedAsBase()
        try test(false)
    } catch let kmd as KnownMostDerived {
        try test(kmd.b == "KnownMostDerived.b")
        try test(kmd.ki == "KnownMostDerived.ki")
        try test(kmd.kmd == "KnownMostDerived.kmd")
    }
    output.writeLine("ok")

    output.write("slicing of known most derived as base (AMI)... ")
    try Promise<Void> { seal in
        firstly {
            testPrx.knownMostDerivedAsBaseAsync()
        }.done {
            try test(false)
        }.catch { e in
            do {
                if let kmd = e as? KnownMostDerived {
                    try test(kmd.b == "KnownMostDerived.b")
                    try test(kmd.ki == "KnownMostDerived.ki")
                    try test(kmd.kmd == "KnownMostDerived.kmd")
                } else {
                    try test(false)
                }
                seal.fulfill(())
            } catch {
                seal.reject(error)
            }
        }
    }.wait()
    output.writeLine("ok")

    output.write("non-slicing of known intermediate as intermediate... ")
    do {
        try testPrx.knownIntermediateAsKnownIntermediate()
        try test(false)
    } catch let ki as KnownIntermediate {
        try test(ki.b == "KnownIntermediate.b")
        try test(ki.ki == "KnownIntermediate.ki")
    }
    output.writeLine("ok")

    output.write("non-slicing of known intermediate as intermediate (AMI)... ")
    try Promise<Void> { seal in
        firstly {
            testPrx.knownIntermediateAsKnownIntermediateAsync()
        }.done {
            try test(false)
        }.catch { e in
            do {
                if let ki = e as? KnownIntermediate {
                    try test(ki.b == "KnownIntermediate.b")
                    try test(ki.ki == "KnownIntermediate.ki")
                } else {
                    try test(false)
                }
                seal.fulfill(())
            } catch {
                seal.reject(error)
            }
        }
    }.wait()
    output.writeLine("ok")

    output.write("non-slicing of known most derived as intermediate... ")
    do {
        try testPrx.knownMostDerivedAsKnownIntermediate()
        try test(false)
    } catch let kmd as KnownMostDerived {
        try test(kmd.b == "KnownMostDerived.b")
        try test(kmd.ki == "KnownMostDerived.ki")
        try test(kmd.kmd == "KnownMostDerived.kmd")
    }
    output.writeLine("ok")

    output.write("non-slicing of known most derived as intermediate (AMI)... ")
    try Promise<Void> { seal in
        firstly {
            testPrx.knownMostDerivedAsKnownIntermediateAsync()
        }.done {
            try test(false)
        }.catch { e in
            do {
                if let kmd = e as? KnownMostDerived {
                    try test(kmd.b == "KnownMostDerived.b")
                    try test(kmd.ki == "KnownMostDerived.ki")
                    try test(kmd.kmd == "KnownMostDerived.kmd")
                } else {
                    try test(false)
                }
                seal.fulfill(())
            } catch {
                seal.reject(error)
            }
        }
    }.wait()
    output.writeLine("ok")

    output.write("non-slicing of known most derived as most derived... ")
    do {
        try testPrx.knownMostDerivedAsKnownMostDerived()
        try test(false)
    } catch let kmd as KnownMostDerived {
        try test(kmd.b == "KnownMostDerived.b")
        try test(kmd.ki == "KnownMostDerived.ki")
        try test(kmd.kmd == "KnownMostDerived.kmd")
    }
    output.writeLine("ok")

    output.write("non-slicing of known most derived as most derived (AMI)... ")
    try Promise<Void> { seal in
        firstly {
            testPrx.knownMostDerivedAsKnownMostDerivedAsync()
        }.done {
            try test(false)
        }.catch { e in
            do {
                if let kmd = e as? KnownMostDerived {
                    try test(kmd.b == "KnownMostDerived.b")
                    try test(kmd.ki == "KnownMostDerived.ki")
                    try test(kmd.kmd == "KnownMostDerived.kmd")
                } else {
                    try test(false)
                }
                seal.fulfill(())
            } catch {
                seal.reject(error)
            }
        }
    }.wait()
    output.writeLine("ok")

    output.write("slicing of unknown most derived, known intermediate as base... ")
    do {
        try testPrx.unknownMostDerived1AsBase()
        try test(false)
    } catch let ki as KnownIntermediate {
        try test(ki.b == "UnknownMostDerived1.b")
        try test(ki.ki == "UnknownMostDerived1.ki")
    }
    output.writeLine("ok")

    output.write("slicing of unknown most derived, known intermediate as base (AMI)... ")
    try Promise<Void> { seal in
        firstly {
            testPrx.unknownMostDerived1AsBaseAsync()
        }.done {
            try test(false)
        }.catch { e in
            do {
                if let ki = e as? KnownIntermediate {
                    try test(ki.b == "UnknownMostDerived1.b")
                    try test(ki.ki == "UnknownMostDerived1.ki")
                } else {
                    try test(false)
                }
                seal.fulfill(())
            } catch {
                seal.reject(error)
            }
        }
    }.wait()
    output.writeLine("ok")

    output.write("slicing of unknown most derived, known intermediate as intermediate... ")
    do {
        try testPrx.unknownMostDerived1AsKnownIntermediate()
        try test(false)
    } catch let ki as KnownIntermediate {
        try test(ki.b == "UnknownMostDerived1.b")
        try test(ki.ki == "UnknownMostDerived1.ki")
    }
    output.writeLine("ok")

    output.write("slicing of unknown most derived, known intermediate as intermediate (AMI)... ")
    try Promise<Void> { seal in
        firstly {
            testPrx.unknownMostDerived1AsKnownIntermediateAsync()
        }.done {
            try test(false)
        }.catch { e in
            do {
                if let ki = e as? KnownIntermediate {
                    try test(ki.b == "UnknownMostDerived1.b")
                    try test(ki.ki == "UnknownMostDerived1.ki")
                } else {
                    try test(false)
                }
                seal.fulfill(())
            } catch {
                seal.reject(error)
            }
        }
    }.wait()
    output.writeLine("ok")

    output.write("slicing of unknown most derived, unknown intermediate thrown as base... ")
    do {
        try testPrx.unknownMostDerived2AsBase()
        try test(false)
    } catch let b as Base {
        try test(b.b == "UnknownMostDerived2.b")
    }
    output.writeLine("ok")

    output.write("slicing of unknown most derived, unknown intermediate thrown as base (AMI)... ")
    try Promise<Void> { seal in
        firstly {
            testPrx.unknownMostDerived2AsBaseAsync()
        }.done {
            try test(false)
        }.catch { e in
            do {
                if let b = e as? Base {
                    try test(b.b == "UnknownMostDerived2.b")
                } else {
                    try test(false)
                }
                seal.fulfill(())
            } catch {
                seal.reject(error)
            }
        }
    }.wait()
    output.writeLine("ok")

    return testPrx
}
