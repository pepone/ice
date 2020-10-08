//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#pragma once

[[cpp:doxygen:include:IceLocatorDiscovery/IceLocatorDiscovery.h]]
[[cpp:header-ext:h]]

[[suppress-warning:reserved-identifier]]
[[js:module:ice]]

[[python:pkgdir:IceLocatorDiscovery]]

#include <Ice/Locator.ice>

[[java:package:com.zeroc]]
[cs:namespace:ZeroC]
/// IceLocatorDiscovery is an Ice plug-in that enables the discovery of IceGrid and custom locators via
/// UDP multicast.
/// The IceDiscovery plug-in implements the {@see Ice.Locator} interface to locate (or discover) locators such as
/// the IceGrid registry or custom IceGrid-like locator implementations using UDP multicast.
module IceLocatorDiscovery
{
    /// The IceLocatorDiscovery.Reply object adapter of a client application hosts a LookupReply object that processes
    /// replies to discovery requests.
    interface LookupReply
    {
        /// Provides a locator proxy in response to a {@see ILookup.FindLocator} call on a Lookup object.
        /// @param proxy The proxy for the locator object.
        void foundLocator(Ice::Locator proxy);
    }

    /// An locator implementation such as IceGrid registry hosts a Lookup object that receives discovery requests from
    /// clients. This Lookup object is well-known object with identity `Ice/LocatorLookup'.
    interface Lookup
    {
        /// Finds a locator with the given instance name.
        /// @param instanceName Restrict the search to locator implementations configured with the given instance name.
        /// If empty, all available locator implementations will reply.
        /// @param reply The reply object to use to send the reply.
        idempotent void findLocator(string instanceName, LookupReply reply);
    }
}
