//
// Copyright (c) ZeroC, Inc. All rights reserved.
//
//
// Ice version 3.7.10
//
// <auto-generated>
//
// Generated from file `Types.ice'
//
// Warning: do not edit this file.
//
// </auto-generated>
//

package com.zeroc.IceBT;

/**
 * Indicates a failure in the Bluetooth plug-in.
 **/
public class BluetoothException extends com.zeroc.Ice.LocalException
{
    public BluetoothException()
    {
        this.reason = "";
    }

    public BluetoothException(Throwable cause)
    {
        super(cause);
        this.reason = "";
    }

    public BluetoothException(String reason)
    {
        this.reason = reason;
    }

    public BluetoothException(String reason, Throwable cause)
    {
        super(cause);
        this.reason = reason;
    }

    public String ice_id()
    {
        return "::IceBT::BluetoothException";
    }

    /**
     * Provides more information about the failure.
     **/
    public String reason;

    /** @hidden */
    public static final long serialVersionUID = -2714934297616925968L;
}
