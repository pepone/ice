<?php
//
// Copyright (c) ZeroC, Inc. All rights reserved.
//
//
// Ice version 3.7.10
//
// <auto-generated>
//
// Generated from file `EndpointF.ice'
//
// Warning: do not edit this file.
//
// </auto-generated>
//


namespace Ice
{
    global $Ice__t_EndpointInfo;
    if(!isset($Ice__t_EndpointInfo))
    {
        $Ice__t_EndpointInfo = IcePHP_declareClass('::Ice::EndpointInfo');
    }
}

namespace Ice
{
    global $Ice__t_IPEndpointInfo;
    if(!isset($Ice__t_IPEndpointInfo))
    {
        $Ice__t_IPEndpointInfo = IcePHP_declareClass('::Ice::IPEndpointInfo');
    }
}

namespace Ice
{
    global $Ice__t_TCPEndpointInfo;
    if(!isset($Ice__t_TCPEndpointInfo))
    {
        $Ice__t_TCPEndpointInfo = IcePHP_declareClass('::Ice::TCPEndpointInfo');
    }
}

namespace Ice
{
    global $Ice__t_UDPEndpointInfo;
    if(!isset($Ice__t_UDPEndpointInfo))
    {
        $Ice__t_UDPEndpointInfo = IcePHP_declareClass('::Ice::UDPEndpointInfo');
    }
}

namespace Ice
{
    global $Ice__t_WSEndpointInfo;
    if(!isset($Ice__t_WSEndpointInfo))
    {
        $Ice__t_WSEndpointInfo = IcePHP_declareClass('::Ice::WSEndpointInfo');
    }
}

namespace Ice
{
    global $Ice__t_Endpoint;
    if(!isset($Ice__t_Endpoint))
    {
        $Ice__t_Endpoint = IcePHP_declareClass('::Ice::Endpoint');
    }
}

namespace Ice
{
    global $Ice__t_EndpointSeq;

    if(!isset($Ice__t_EndpointSeq))
    {
        global $Ice__t_Endpoint;
        $Ice__t_EndpointSeq = IcePHP_defineSequence('::Ice::EndpointSeq', $Ice__t_Endpoint);
    }
}
?>
