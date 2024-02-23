//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef TEST_I_H
#define TEST_I_H

#include <Test.h>

class TestI final : public Test::TestIntf
{
public:

    void transient(const Ice::Current&);
    void deactivate(const Ice::Current&);
};

class Cookie
{
public:

    std::string message() const;
};

#endif
