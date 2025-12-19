// Copyright (c) ZeroC, Inc.

pluginManagement {
    includeBuild("./build-logic")
    includeBuild("./tools/slice-tools")
    repositories {
        gradlePluginPortal()
        mavenCentral()
    }
}

dependencyResolutionManagement {
    repositories {
        mavenCentral()
    }
}

// Library projects
val libraryProjects = listOf(
    "ice",
    "icediscovery",
    "icelocatordiscovery",
    "icebt",
    "icebox",
    "glacier2",
    "icestorm",
    "icegrid"
)

libraryProjects.forEach {
    include(":$it")
    project(":$it").projectDir = file("src/com.zeroc.$it")
}

// Application projects
include(":IceGridGUI")
project(":IceGridGUI").projectDir = file("src/IceGridGUI")

// Tests
include(":test")
include(":testPlugins")
project(":testPlugins").projectDir = file("test/plugins")
