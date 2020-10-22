//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#pragma once

[[java:package(test.Ice.packagemd)]]
module Test1
{
class C1
{
    int i;
}

class C2 : C1
{
    long l;
}

exception E1
{
    int i;
}

exception E2 : E1
{
    long l;
}

exception notify /* Test keyword escape. */
{
    int i;
}
}
