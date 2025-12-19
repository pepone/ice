// Copyright (c) ZeroC, Inc.

pluginManagement {
    includeBuild("./build-logic")
    includeBuild("./tools/slice-tools")
}

dependencyResolutionManagement {
    repositories {
        mavenCentral()
    }
}

// Source projects
include(":ice")
project(":ice").projectDir = file("src/com.zeroc.ice")

include(":icediscovery")
project(":icediscovery").projectDir = file("src/com.zeroc.icediscovery")

include(":icelocatordiscovery")
project(":icelocatordiscovery").projectDir = file("src/com.zeroc.icelocatordiscovery")

include(":icebt")
project(":icebt").projectDir = file("src/com.zeroc.icebt")

include(":icebox")
project(":icebox").projectDir = file("src/com.zeroc.icebox")

include(":glacier2")
project(":glacier2").projectDir = file("src/com.zeroc.glacier2")

include(":icestorm")
project(":icestorm").projectDir = file("src/com.zeroc.icestorm")

include(":icegrid")
project(":icegrid").projectDir = file("src/com.zeroc.icegrid")

include(":IceGridGUI")
project(":IceGridGUI").projectDir = file("src/IceGridGUI")

// Tests
include(":test")

include(":testPlugins")
project(":testPlugins").projectDir = file("test/plugins")
