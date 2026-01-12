// Copyright (c) ZeroC, Inc.

package com.zeroc.slice.tools

import org.gradle.api.Action
import org.gradle.api.tasks.SourceSet

/**
 * Kotlin DSL accessor for the slice source set extension.
 * Enables the syntax: sourceSets { main { slice { ... } } }
 */
fun SourceSet.slice(action: Action<SliceSourceSet>) {
    val sliceSourceSet = extensions.getByName("slice") as SliceSourceSet
    action.execute(sliceSourceSet)
}

/**
 * Kotlin DSL accessor for getting the slice source set.
 * Enables the syntax: sourceSets.main.slice
 */
val SourceSet.slice: SliceSourceSet
    get() = extensions.getByName("slice") as SliceSourceSet
