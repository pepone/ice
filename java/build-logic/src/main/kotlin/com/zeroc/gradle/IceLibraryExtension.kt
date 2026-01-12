// Copyright (c) ZeroC, Inc.

package com.zeroc.gradle

import org.gradle.api.provider.Property

/**
 * Extension for configuring Ice library metadata.
 */
interface IceLibraryExtension {
    /** The display name of the library (e.g., "Ice" or "IceGrid") */
    val displayName: Property<String>

    /** The Java module name (e.g., "com.zeroc.ice") */
    val moduleName: Property<String>

    /** The library description */
    val description: Property<String>
}
