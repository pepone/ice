// Copyright (c) ZeroC, Inc.

#pragma once

#include "First.ice"
#include "Second.ice"

module Outer
{
    module Inner
    {
        // Combined struct that uses types from both First.ice and Second.ice
        // This tests that both First and Second are accessible in the same nested module
        struct Combined
        {
            First first;
            Second second;
        }

        module Deep
        {
            // DeepCombined struct uses types from deep nested modules in both First.ice and Second.ice
            // This tests that multi-level nested module merging works correctly
            struct DeepCombined
            {
                DeepFirst deepFirst;
                DeepSecond deepSecond;
            }
        }
    }
}
