// Copyright (c) ZeroC, Inc.

package com.zeroc.ice.gradle

import org.gradle.api.provider.Property

/**
 * Extension for configuring Ice library module metadata.
 */
abstract class IceLibraryExtension {
    abstract val displayName: Property<String>
    abstract val moduleName: Property<String>
    abstract val projectDescription: Property<String>
}
