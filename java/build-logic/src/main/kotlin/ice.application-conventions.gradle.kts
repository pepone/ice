// Copyright (c) ZeroC, Inc.

/**
 * Convention plugin for Ice Java application modules (non-library modules like IceGridGUI, test).
 * Applies java plugin with common Ice configuration but without library-specific setup.
 */

plugins {
    id("ice.java-conventions")
}

// Application modules don't need most library conveniences, just the base configuration
