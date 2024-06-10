// Copyright (c) ZeroC, Inc.
#import "Convert.h"
#import "Exception.h"
#import "IceUtil.h"

NSError*
convertException(std::exception_ptr exc)
{
    assert(exc);
    Class<ICEExceptionFactory> factory = [ICEUtil exceptionFactory];

    try
    {
        rethrow_exception(exc);
    }
    catch (const Ice::InitializationException& e)
    {
        return [factory initializationException:toNSString(e.reason) file:toNSString(e.ice_file()) line:e.ice_line()];
    }
    catch (const Ice::PluginInitializationException& e)
    {
        return [factory pluginInitializationException:toNSString(e.reason)
                                                 file:toNSString(e.ice_file())
                                                 line:e.ice_line()];
    }
    catch (const Ice::AlreadyRegisteredException& e)
    {
        return [factory alreadyRegisteredException:toNSString(e.kindOfObject)
                                                id:toNSString(e.id)
                                              file:toNSString(e.ice_file())
                                              line:e.ice_line()];
    }
    catch (const Ice::NotRegisteredException& e)
    {
        return [factory notRegisteredException:toNSString(e.kindOfObject)
                                            id:toNSString(e.id)
                                          file:toNSString(e.ice_file())
                                          line:e.ice_line()];
    }
    catch (const Ice::TwowayOnlyException& e)
    {
        return [factory twowayOnlyException:toNSString(e.operation) file:toNSString(e.ice_file()) line:e.ice_line()];
    }
    catch (const Ice::CommunicatorDestroyedException& e)
    {
        return [factory communicatorDestroyedException:toNSString(e.ice_file()) line:e.ice_line()];
    }
    catch (const Ice::ObjectAdapterDeactivatedException& e)
    {
        return [factory objectAdapterDeactivatedException:toNSString(e.name)
                                                     file:toNSString(e.ice_file())
                                                     line:e.ice_line()];
    }
    catch (const Ice::ObjectAdapterIdInUseException& e)
    {
        return [factory objectAdapterIdInUseException:toNSString(e.id) file:toNSString(e.ice_file()) line:e.ice_line()];
    }
    catch (const Ice::NoEndpointException& e)
    {
        return [factory noEndpointException:toNSString(e.proxy) file:toNSString(e.ice_file()) line:e.ice_line()];
    }
    catch (const Ice::EndpointParseException& e)
    {
        return [factory endpointParseException:toNSString(e.str) file:toNSString(e.ice_file()) line:e.ice_line()];
    }
    catch (const Ice::EndpointSelectionTypeParseException& e)
    {
        return [factory endpointSelectionTypeParseException:toNSString(e.str)
                                                       file:toNSString(e.ice_file())
                                                       line:e.ice_line()];
    }
    catch (const Ice::VersionParseException& e)
    {
        return [factory versionParseException:toNSString(e.str) file:toNSString(e.ice_file()) line:e.ice_line()];
    }
    catch (const Ice::IdentityParseException& e)
    {
        return [factory identityParseException:toNSString(e.str) file:toNSString(e.ice_file()) line:e.ice_line()];
    }
    catch (const Ice::ProxyParseException& e)
    {
        return [factory proxyParseException:toNSString(e.str) file:toNSString(e.ice_file()) line:e.ice_line()];
    }
    catch (const Ice::IllegalIdentityException& e)
    {
        return [factory illegalIdentityException:toNSString(e.ice_file()) line:e.ice_line()];
    }
    catch (const Ice::IllegalServantException& e)
    {
        return [factory illegalServantException:toNSString(e.reason) file:toNSString(e.ice_file()) line:e.ice_line()];
    }
    catch (const Ice::DNSException& e)
    {
        return [factory dNSException:e.error host:toNSString(e.host) file:toNSString(e.ice_file()) line:e.ice_line()];
    }
    catch (const Ice::InvocationCanceledException& e)
    {
        return [factory invocationCanceledException:toNSString(e.ice_file()) line:e.ice_line()];
    }
    catch (const Ice::FeatureNotSupportedException& e)
    {
        return [factory featureNotSupportedException:toNSString(e.unsupportedFeature)
                                                file:toNSString(e.ice_file())
                                                line:e.ice_line()];
    }
    catch (const Ice::FixedProxyException& e)
    {
        return [factory fixedProxyException:toNSString(e.ice_file()) line:e.ice_line()];
    }
    catch (const Ice::SecurityException& e)
    {
        return [factory securityException:toNSString(e.reason) file:toNSString(e.ice_file()) line:e.ice_line()];
    }
    catch (const Ice::UnknownLocalException& e)
    {
        return [factory unknownLocalException:toNSString(e.unknown) file:toNSString(e.ice_file()) line:e.ice_line()];
    }
    catch (const Ice::UnknownUserException& e)
    {
        return [factory unknownUserException:toNSString(e.unknown) file:toNSString(e.ice_file()) line:e.ice_line()];
    }
    catch (const Ice::UnknownException& e)
    {
        return [factory unknownException:toNSString(e.unknown) file:toNSString(e.ice_file()) line:e.ice_line()];
    }
    catch (const Ice::ObjectNotExistException& e)
    {
        return [factory objectNotExistException:toNSString(e.id.name)
                                       category:toNSString(e.id.category)
                                          facet:toNSString(e.facet)
                                      operation:toNSString(e.operation)
                                           file:toNSString(e.ice_file())
                                           line:e.ice_line()];
    }
    catch (const Ice::FacetNotExistException& e)
    {
        return [factory facetNotExistException:toNSString(e.id.name)
                                      category:toNSString(e.id.category)
                                         facet:toNSString(e.facet)
                                     operation:toNSString(e.operation)
                                          file:toNSString(e.ice_file())
                                          line:e.ice_line()];
    }
    catch (const Ice::OperationNotExistException& e)
    {
        return [factory operationNotExistException:toNSString(e.id.name)
                                          category:toNSString(e.id.category)
                                             facet:toNSString(e.facet)
                                         operation:toNSString(e.operation)
                                              file:toNSString(e.ice_file())
                                              line:e.ice_line()];
    }
    catch (const Ice::RequestFailedException& e)
    {
        return [factory requestFailedException:toNSString(e.id.name)
                                      category:toNSString(e.id.category)
                                         facet:toNSString(e.facet)
                                     operation:toNSString(e.operation)
                                          file:toNSString(e.ice_file())
                                          line:e.ice_line()];
    }
    catch (const Ice::ConnectionRefusedException& e)
    {
        return [factory connectionRefusedException:e.error file:toNSString(e.ice_file()) line:e.ice_line()];
    }
    catch (const Ice::FileException& e)
    {
        return [factory fileException:e.error path:toNSString(e.path) file:toNSString(e.ice_file()) line:e.ice_line()];
    }
    catch (const Ice::ConnectFailedException& e)
    {
        return [factory connectFailedException:e.error file:toNSString(e.ice_file()) line:e.ice_line()];
    }
    catch (const Ice::ConnectionLostException& e)
    {
        return [factory connectionLostException:e.error file:toNSString(e.ice_file()) line:e.ice_line()];
    }
    catch (const Ice::SocketException& e)
    {
        return [factory socketException:e.error file:toNSString(e.ice_file()) line:e.ice_line()];
    }
    catch (const Ice::SyscallException& e)
    {
        return [factory syscallException:e.error file:toNSString(e.ice_file()) line:e.ice_line()];
    }
    catch (const Ice::ConnectTimeoutException& e)
    {
        return [factory connectTimeoutException:toNSString(e.ice_file()) line:e.ice_line()];
    }
    catch (const Ice::CloseTimeoutException& e)
    {
        return [factory closeTimeoutException:toNSString(e.ice_file()) line:e.ice_line()];
    }
    catch (const Ice::ConnectionIdleException& e)
    {
        return [factory connectionIdleException:toNSString(e.ice_file()) line:e.ice_line()];
    }
    catch (const Ice::InvocationTimeoutException& e)
    {
        return [factory invocationTimeoutException:toNSString(e.ice_file()) line:e.ice_line()];
    }
    catch (const Ice::TimeoutException& e)
    {
        return [factory timeoutException:toNSString(e.ice_file()) line:e.ice_line()];
    }
    catch (const Ice::BadMagicException& e)
    {
        NSData* badMagic = [[NSData alloc] initWithBytes:e.badMagic.data() length:e.badMagic.size()];
        return [factory badMagicException:toNSString(e.reason)
                                 badMagic:badMagic
                                     file:toNSString(e.ice_file())
                                     line:e.ice_line()];
    }
    catch (const Ice::UnsupportedProtocolException& e)
    {
        return [factory unsupportedProtocolException:toNSString(e.reason)
                                            badMajor:e.bad.major
                                            badMinor:e.bad.minor
                                      supportedMajor:e.supported.major
                                      supportedMinor:e.supported.minor
                                                file:toNSString(e.ice_file())
                                                line:e.ice_line()];
    }
    catch (const Ice::UnsupportedEncodingException& e)
    {
        return [factory unsupportedEncodingException:toNSString(e.reason)
                                            badMajor:e.bad.major
                                            badMinor:e.bad.minor
                                      supportedMajor:e.supported.major
                                      supportedMinor:e.supported.minor
                                                file:toNSString(e.ice_file())
                                                line:e.ice_line()];
    }
    catch (const Ice::UnknownMessageException& e)
    {
        return [factory unknownMessageException:toNSString(e.reason) file:toNSString(e.ice_file()) line:e.ice_line()];
    }
    catch (const Ice::ConnectionNotValidatedException& e)
    {
        return [factory connectionNotValidatedException:toNSString(e.reason)
                                                   file:toNSString(e.ice_file())
                                                   line:e.ice_line()];
    }
    catch (const Ice::UnknownReplyStatusException& e)
    {
        return [factory unknownReplyStatusException:toNSString(e.reason)
                                               file:toNSString(e.ice_file())
                                               line:e.ice_line()];
    }
    catch (const Ice::CloseConnectionException& e)
    {
        return [factory closeConnectionException:toNSString(e.reason) file:toNSString(e.ice_file()) line:e.ice_line()];
    }
    catch (const Ice::ConnectionManuallyClosedException& e)
    {
        return [factory connectionManuallyClosedException:e.graceful file:toNSString(e.ice_file()) line:e.ice_line()];
    }
    catch (const Ice::IllegalMessageSizeException& e)
    {
        return [factory illegalMessageSizeException:toNSString(e.reason)
                                               file:toNSString(e.ice_file())
                                               line:e.ice_line()];
    }
    catch (const Ice::CompressionException& e)
    {
        return [factory compressionException:toNSString(e.reason) file:toNSString(e.ice_file()) line:e.ice_line()];
    }
    catch (const Ice::DatagramLimitException& e)
    {
        return [factory datagramLimitException:toNSString(e.reason) file:toNSString(e.ice_file()) line:e.ice_line()];
    }
    catch (const Ice::ProxyUnmarshalException& e)
    {
        return [factory proxyUnmarshalException:toNSString(e.reason) file:toNSString(e.ice_file()) line:e.ice_line()];
    }
    catch (const Ice::UnmarshalOutOfBoundsException& e)
    {
        return [factory unmarshalOutOfBoundsException:toNSString(e.reason)
                                                 file:toNSString(e.ice_file())
                                                 line:e.ice_line()];
    }
    catch (const Ice::NoValueFactoryException& e)
    {
        return [factory noValueFactoryException:toNSString(e.reason)
                                           type:toNSString(e.type)
                                           file:toNSString(e.ice_file())
                                           line:e.ice_line()];
    }
    catch (const Ice::UnexpectedObjectException& e)
    {
        return [factory unexpectedObjectException:toNSString(e.reason)
                                             type:toNSString(e.type)
                                     expectedType:toNSString(e.expectedType)
                                             file:toNSString(e.ice_file())
                                             line:e.ice_line()];
    }
    catch (const Ice::MemoryLimitException& e)
    {
        return [factory memoryLimitException:toNSString(e.reason) file:toNSString(e.ice_file()) line:e.ice_line()];
    }
    catch (const Ice::EncapsulationException& e)
    {
        return [factory encapsulationException:toNSString(e.reason) file:toNSString(e.ice_file()) line:e.ice_line()];
    }
    catch (const Ice::MarshalException& e)
    {
        return [factory marshalException:toNSString(e.reason) file:toNSString(e.ice_file()) line:e.ice_line()];
    }
    catch (const Ice::ProtocolException& e)
    {
        return [factory protocolException:toNSString(e.reason) file:toNSString(e.ice_file()) line:e.ice_line()];
    }
    catch (const Ice::LocalException& e)
    {
        return [factory localException:toNSString(e.ice_file()) line:e.ice_line()];
    }
    catch (const std::exception& e)
    {
        return [factory runtimeError:toNSString(e.what())];
    }
    catch (...)
    {
        return [factory runtimeError:toNSString("unknown c++ exception")];
    }
}

NSObject*
toObjC(const std::shared_ptr<Ice::Endpoint>& endpoint)
{
    return [ICEEndpoint getHandle:endpoint];
}

void
fromObjC(id object, std::shared_ptr<Ice::Endpoint>& endpoint)
{
    ICEEndpoint* endpt = object;
    endpoint = object == [NSNull null] ? nullptr : [endpt endpoint];
}
