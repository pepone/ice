//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

import Foundation
import Ice
import PromiseKit
import TestCommon

class TestI: TestIntf {
    var _batchCount: Int32
    var _shutdown: Bool
    var _lock = os_unfair_lock()
    var _semaphore = DispatchSemaphore(value: 0)
    var _pending: Resolver<Void>?
    var _helper: TestHelper

    init(helper: TestHelper) {
        _batchCount = 0
        _shutdown = false
        _helper = helper
    }

    func op(current _: Current) throws {}

    func opWithPayload(seq _: ByteSeq, current _: Current) throws {}

    func opWithResult(current _: Current) throws -> Int32 {
        return 15
    }

    func opWithUE(current _: Current) throws {
        throw TestIntfException()
    }

    func opWithResultAndUE(current _: Current) throws -> Int32 {
        throw TestIntfException()
    }

    func opWithArgs(current _: Current) throws -> (
        one: Int32,
        two: Int32,
        three: Int32,
        four: Int32,
        five: Int32,
        six: Int32,
        seven: Int32,
        eight: Int32,
        nine: Int32,
        ten: Int32,
        eleven: Int32
    ) {
        return (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11)
    }

    func startDispatchAsync(current _: Current) -> Promise<Void> {
        return withLock(&_lock) {
            if _shutdown {
                return Promise.value(())
            } else if let pending = _pending {
                pending.fulfill(())
            }
            return Promise<Void> { seal in
                _pending = seal
            }
        }
    }

    func pingBiDir(reply: PingReplyPrx?, current: Current) throws {
        if let reply = reply {
            try reply.ice_fixed(current.con!).replyAsync().wait()
        }
    }

    func opBatch(current _: Current) throws {
        withLock(&_lock) {
            _batchCount += 1
            _semaphore.signal()
        }
    }

    func opBatchCount(current _: Current) throws -> Int32 {
        return withLock(&_lock) {
            _batchCount
        }
    }

    func waitForBatch(count: Int32, current _: Current) throws -> Bool {
        while _batchCount < count {
            if _semaphore.wait(timeout: .now() + .seconds(5)) == .timedOut {
                try _helper.test(false)
            }
        }
        let result = count == _batchCount
        _batchCount = 0
        return result
    }

    func close(mode: CloseMode, current: Current) throws {
        if let con = current.con,
            let closeMode = ConnectionClose(rawValue: mode.rawValue)
        {
            try con.close(closeMode)
        }
    }

    func sleep(ms: Int32, current _: Current) throws {
        withLock(&_lock) {
            Thread.sleep(forTimeInterval: TimeInterval(ms) / 1000)
        }
    }

    func finishDispatch(current _: Current) throws {
        withLock(&_lock) {
            if _shutdown {
                return
            } else if let pending = _pending {
                // Pending might not be set yet if startDispatch is dispatch out-of-order
                pending.fulfill(())
                _pending = nil
            }
        }
    }

    func shutdown(current: Current) throws {
        withLock(&_lock) {
            _shutdown = true
            if let pending = _pending {
                // Pending might not be set yet if startDispatch is dispatch out-of-order
                pending.fulfill(())
                _pending = nil
            }
        }
        current.adapter.getCommunicator().shutdown()
    }

    func supportsAMD(current _: Current) throws -> Bool {
        return true
    }

    func supportsFunctionalTests(current _: Current) throws -> Bool {
        return false
    }
}

class TestII: OuterInnerTestIntf {
    func op(i: Int32, current _: Ice.Current) throws -> (returnValue: Int32, j: Int32) {
        return (i, i)
    }
}

class TestControllerI: TestIntfController {
    var _adapter: Ice.ObjectAdapter

    init(adapter: Ice.ObjectAdapter) {
        _adapter = adapter
    }

    func holdAdapter(current _: Ice.Current) {
        _adapter.hold()
    }

    func resumeAdapter(current _: Ice.Current) throws {
        try _adapter.activate()
    }
}
