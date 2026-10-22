// Copyright (c) ZeroC, Inc.

// Parses the input as an RFC 2253 distinguished name, as Ice::SSL does with the subject name of a peer's certificate.

#include "../src/Ice/SSL/DistinguishedName.h"
#include "Ice/LocalExceptions.h"

#include <cstddef>
#include <cstdint>
#include <string>

extern "C" int
LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    try
    {
        Ice::SSL::DistinguishedName dn{std::string(reinterpret_cast<const char*>(data), size)};
        (void)std::string(dn);
    }
    catch (const Ice::ParseException&)
    {
    }
    return 0;
}
