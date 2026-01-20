// Copyright (c) ZeroC, Inc.

#pragma once

module Outer
{
    module Inner
    {
        struct First
        {
            int value;
        }

        module Deep
        {
            struct DeepFirst
            {
                int deepValue;
            }
        }
    }
}
