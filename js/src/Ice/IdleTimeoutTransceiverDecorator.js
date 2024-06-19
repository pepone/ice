//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

import { Debug } from "./Debug";
import { SocketOperation } from "./SocketOperation.js";

export class IdleTimeoutTransceiverDecorator {
    toString() {
        return this._decoratee.ToString();
    }

    checkSendSize(buf) {
        return this._decoratee.checkSendSize(buf);
    }

    close() {
        this.cancelReadTimer();
        this.cancelWriteTimer();
        this._decoratee.close();
    }

    getInfo() {
        this._decoratee.getInfo();
    }

    initialize(readBuffer, writeBuffer) {
        const op = this._decoratee.initialize(readBuffer, writeBuffer);
        if (op == SocketOperation.None) {
            // connected
            this.rescheduleReadTimer();
            this.rescheduleWriteTimer();
        }
        return op;
    }

    read(buf, moreData) {
        // We don't want the idle check to run while we're reading, so we reschedule it before reading.
        this.rescheduleReadTimer();
        return this._decoratee.read(buf, moreData);
    }

    setBufferSize(rcvSize, sndSize) {
        this._decoratee.setBufferSize(rcvSize, sndSize);
    }

    write(buf) {
        this.cancelWriteTimer();
        const op = this._decoratee.write(buf);
        if (op == SocketOperation.None) {
            // write completed
            this.rescheduleWriteTimer();
        }
        return op;
    }

    constructor(decoratee, connection, timer, idleTimeout, enableIdleCheck) {
        Debug.Assert(idleTimeout > 0);

        this._decoratee = decoratee;
        this._idleTimeout = idleTimeout;
        this._timer = timer;

        if (enableIdleCheck) {
            this._readTimerToken = this._timer.schedule(() => connection.idleCheck(_idleTimeout));
        }

        this._writeTimerToken = this._timer.schedule((_) => connection.sendHeartbeat());
    }

    cancelReadTimer() {}

    cancelWriteTimer() {}

    rescheduleReadTimer() {}

    rescheduleWriteTimer() {}
}
