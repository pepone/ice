// Copyright (c) ZeroC, Inc.

// Decodes the input as Base64, as WSTransceiver does with the Sec-WebSocket-Key header a peer sends, and checks that
// re-encoding what was decoded gives back a string that decodes to the same bytes.

#include "../src/Ice/Base64.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <vector>

using namespace IceInternal;

extern "C" int
LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    std::vector<std::byte> decoded;
    try
    {
        decoded = Base64::decode(std::string(reinterpret_cast<const char*>(data), size));
    }
    catch (const std::invalid_argument&)
    {
        return 0;
    }

    if (Base64::decode(Base64::encode(decoded)) != decoded)
    {
        std::abort();
    }
    return 0;
}
