// Copyright (c) ZeroC, Inc.

package com.zeroc.gradle

import org.gradle.api.JavaVersion
import org.gradle.api.Plugin
import org.gradle.api.Project
import org.gradle.api.plugins.JavaPluginExtension
import org.gradle.api.tasks.compile.JavaCompile
import org.gradle.api.tasks.javadoc.Javadoc
import org.gradle.external.javadoc.StandardJavadocDocletOptions
import org.gradle.jvm.toolchain.JavaLanguageVersion

/**
 * Base plugin for all Ice Java projects.
 * Configures common settings like Java version, encoding, and compiler options.
 */
class IceBasePlugin : Plugin<Project> {
    override fun apply(project: Project) {
        with(project) {
            pluginManager.apply("java-library")

            // Configure Java toolchain
            extensions.configure<JavaPluginExtension>("java") {
                toolchain {
                    languageVersion.set(JavaLanguageVersion.of(17))
                }
            }

            // Configure Java compilation
            tasks.withType(JavaCompile::class.java).configureEach {
                options.encoding = "UTF-8"
                options.release.set(17)
            }

            // Configure Javadoc
            tasks.withType(Javadoc::class.java).configureEach {
                options.encoding = "UTF-8"
                (options as StandardJavadocDocletOptions).apply {
                    addStringOption("Xdoclint:none", "-quiet")
                    source = "17"
                }
            }
        }
    }
}
