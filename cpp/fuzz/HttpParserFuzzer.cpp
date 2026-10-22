// Copyright (c) ZeroC, Inc.

// Feeds the input to the HTTP parser the way WSTransceiver reads a WebSocket upgrade request or response from a peer.

#include "../src/Ice/HttpParser.h"

#include <cstddef>
#include <cstdint>
#include <string>

using namespace IceInternal;

extern "C" int
LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    const auto* begin = reinterpret_cast<const std::byte*>(data);
    const std::byte* end = begin + size;

    HttpParser parser;
    try
    {
        const std::byte* messageEnd = parser.isCompleteMessage(begin, end);
        if (!messageEnd || !parser.parse(begin, messageEnd))
        {
            return 0;
        }

        // The accessors assert that the parsed message has the matching type.
        if (parser.type() == HttpParser::TypeRequest)
        {
            (void)parser.method();
            (void)parser.uri();
        }
        else if (parser.type() == HttpParser::TypeResponse)
        {
            (void)parser.status();
            (void)parser.reason();
        }
        (void)parser.versionMajor();
        (void)parser.versionMinor();

        std::string value;
        (void)parser.getHeader("Upgrade", value, true);
        (void)parser.getHeader("Connection", value, true);
        (void)parser.getHeader("Sec-WebSocket-Key", value, false);
        (void)parser.getHeaders();
    }
    catch (const WebSocketException&)
    {
    }
    return 0;
}
