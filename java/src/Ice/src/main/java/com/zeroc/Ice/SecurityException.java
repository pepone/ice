//
// Copyright (c) ZeroC, Inc. All rights reserved.
//
//
// Ice version 3.7.10
//
// <auto-generated>
//
// Generated from file `LocalException.ice'
//
// Warning: do not edit this file.
//
// </auto-generated>
//

package com.zeroc.Ice;

/** This exception indicates a failure in a security subsystem, such as the Ice.SSL plug-in. */
public class SecurityException extends LocalException {
  public SecurityException() {
    this.reason = "";
  }

  public SecurityException(Throwable cause) {
    super(cause);
    this.reason = "";
  }

  public SecurityException(String reason) {
    this.reason = reason;
  }

  public SecurityException(String reason, Throwable cause) {
    super(cause);
    this.reason = reason;
  }

  public String ice_id() {
    return "::Ice::SecurityException";
  }

  /** The reason for the failure. */
  public String reason;

  private static final long serialVersionUID = 7929245908983964710L;
}
