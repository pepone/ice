// Copyright (c) ZeroC, Inc.

#pragma once

module Outer
{
    module Inner
    {
        module Transitive
        {
            struct TransitiveFirst
            {
                int transitiveValue;
            }
        }
    }
}
