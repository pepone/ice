// Copyright (c) ZeroC, Inc.

/**
 * Convention plugin for base Ice Java project configuration.
 * Applied to all subprojects for common settings like Checkstyle, Slice tools configuration, etc.
 */

plugins {
    java
    checkstyle
    id("com.zeroc.slice-tools")
}

val runningInCi = providers.environmentVariable("CI").isPresent
val topSrcDir = "${rootProject.projectDir}/.."
val libDir = "${rootProject.projectDir}/lib"

// Store in extra properties for access by other scripts/plugins
extra["topSrcDir"] = topSrcDir
extra["libDir"] = libDir

group = "com.zeroc"

// Configure the slice extension
extensions.configure<com.zeroc.slice.tools.SliceExtension> {
    includeSearchPath.setFrom(files("$topSrcDir/slice"))

    val defaultIceToolsPath = "${file(topSrcDir).canonicalPath}/cpp/bin"
    if (org.gradle.internal.os.OperatingSystem.current().isWindows) {
        val platform = project.findProperty("cppPlatform")?.toString() ?: "x64"
        val configuration = project.findProperty("cppConfiguration")?.toString() ?: "Release"
        toolsPath.set("$defaultIceToolsPath/$platform/$configuration")
    } else {
        toolsPath.set(defaultIceToolsPath)
    }
}

// Configure Checkstyle
checkstyle {
    toolVersion = "10.21.4"

    // If we're running in CI, we want the build to fail if any warnings are emitted.
    if (runningInCi) {
        maxWarnings = 0
    }
}

tasks.withType<Checkstyle>().configureEach {
    // Ensure that running checkstyle multiple times always yields the same output.
    outputs.upToDateWhen { false }
    doNotTrackState("Always run Checkstyle")
}

// Common Java configuration
val targetJavaRelease = rootProject.findProperty("targetJavaRelease")?.toString() ?: "17"

java {
    toolchain {
        languageVersion.set(JavaLanguageVersion.of(targetJavaRelease))
    }
    withSourcesJar()
    withJavadocJar()
}

tasks.withType<Jar>().configureEach {
    manifest {
        attributes("Built-By" to "ZeroC, Inc.")
    }
}

tasks.withType<JavaCompile>().configureEach {
    options.release.set(targetJavaRelease.toInt())
    options.isDebug = (rootProject.findProperty("debug")?.toString() ?: "true").toBoolean()
    options.compilerArgs.addAll(
        listOf(
            "-Xdoclint:all,-missing",
            "-Xlint:all,-rawtypes,-exports,-serial,-try,-missing-explicit-ctor,-deprecation"
        )
    )
    options.encoding = "UTF-8"
    options.isDeprecation = true
}
