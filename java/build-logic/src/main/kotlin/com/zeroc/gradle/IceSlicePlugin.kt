// Copyright (c) ZeroC, Inc.

package com.zeroc.gradle

import com.zeroc.slice.tools.SliceExtension
import org.gradle.api.Plugin
import org.gradle.api.Project
import org.gradle.kotlin.dsl.*

/**
 * Plugin for projects that need Slice compilation.
 * Configures the slice-tools plugin with Ice-specific paths.
 */
class IceSlicePlugin : Plugin<Project> {
    override fun apply(project: Project) {
        with(project) {
            pluginManager.apply("com.zeroc.slice-tools")

            // Configure slice extension with Ice paths
            val topSrcDir = extra["topSrcDir"] as String
            val isWindows = System.getProperty("os.name").lowercase().contains("windows")

            // On Windows, use cppPlatform/cppConfiguration from gradle.properties (default: x64/Release)
            // On macOS/Linux, use cpp/bin directly
            val binDir = if (isWindows) {
                val cppPlatform = findProperty("cppPlatform")?.toString()?.takeIf { it.isNotBlank() } ?: "x64"
                val cppConfiguration = findProperty("cppConfiguration")?.toString()?.takeIf { it.isNotBlank() } ?: "Release"
                "$topSrcDir/cpp/bin/$cppPlatform/$cppConfiguration/"
            } else {
                "$topSrcDir/cpp/bin/"
            }

            extensions.configure<SliceExtension>("slice") {
                toolsPath.set(binDir)
                includeSearchPath.from("$topSrcDir/slice")
            }
        }
    }
}
