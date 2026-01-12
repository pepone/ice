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
            val binDir = if (isWindows) "$topSrcDir/cpp/bin/x64/Release/" else "$topSrcDir/cpp/bin/"

            extensions.configure<SliceExtension>("slice") {
                toolsPath.set(binDir)
                includeSearchPath.from("$topSrcDir/slice")
            }
        }
    }
}
