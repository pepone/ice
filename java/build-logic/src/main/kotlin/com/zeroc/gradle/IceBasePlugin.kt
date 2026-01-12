// Copyright (c) ZeroC, Inc.

package com.zeroc.gradle

import org.gradle.api.Plugin
import org.gradle.api.Project
import org.gradle.api.artifacts.VersionCatalog
import org.gradle.api.artifacts.VersionCatalogsExtension
import org.gradle.api.plugins.JavaPluginExtension
import org.gradle.api.plugins.quality.CheckstyleExtension
import org.gradle.api.tasks.compile.JavaCompile
import org.gradle.api.tasks.javadoc.Javadoc
import org.gradle.external.javadoc.StandardJavadocDocletOptions
import org.gradle.jvm.tasks.Jar
import org.gradle.kotlin.dsl.*

/**
 * Base convention plugin for all Ice Java projects.
 * Configures common settings: version, group, Java compilation, checkstyle, JAR manifest.
 */
class IceBasePlugin : Plugin<Project> {
    override fun apply(project: Project) {
        with(project) {
            // Apply required plugins
            pluginManager.apply("java-library")
            pluginManager.apply("checkstyle")

            // Access version catalog
            val libs = rootProject.extensions.getByType<VersionCatalogsExtension>().named("libs")
            val iceVersion = libs.findVersion("ice").get().requiredVersion
            val javaVersion = libs.findVersion("java").get().requiredVersion.toInt()
            val checkstyleVersion = libs.findVersion("checkstyle").get().requiredVersion

            // Read debug flag from gradle.properties (default: false)
            val debug = findProperty("debug")?.toString()?.toBoolean() ?: false

            // Set version and group
            version = iceVersion
            group = "com.zeroc"

            // Store topSrcDir as extra property for subprojects
            extra["topSrcDir"] = rootProject.projectDir.parentFile.absolutePath

            // Configure Java extension
            extensions.configure<JavaPluginExtension> {
                withSourcesJar()
                withJavadocJar()
            }

            // Configure JAR manifest
            tasks.named<Jar>("jar") {
                manifest {
                    attributes("Built-By" to "ZeroC, Inc.")
                }
            }

            // Configure Java compilation
            tasks.withType<JavaCompile>().configureEach {
                options.encoding = "UTF-8"
                options.release.set(javaVersion)
                options.isDebug = debug
                options.isDeprecation = true
                options.compilerArgs.addAll(
                    listOf(
                        "-Xdoclint:all,-missing",
                        "-Xlint:all,-rawtypes,-exports,-serial,-try,-missing-explicit-ctor,-deprecation"
                    )
                )
            }

            // Configure Javadoc
            tasks.withType<Javadoc>().configureEach {
                options.encoding = "UTF-8"
                (options as StandardJavadocDocletOptions).apply {
                    addStringOption("Xdoclint:none", "-quiet")
                    source = javaVersion.toString()
                }
            }

            // Configure checkstyle
            extensions.configure<CheckstyleExtension> {
                toolVersion = checkstyleVersion
                isIgnoreFailures = false
                isShowViolations = true
            }
        }
    }
}
