// Copyright (c) ZeroC, Inc.

namespace IceInternal;

public interface Connector
{
    // Create a transceiver without blocking. The transceiver may not be fully connected
    // until its initialize method is called.
    Transceiver connect();

    short type();
}
