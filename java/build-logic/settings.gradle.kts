// Copyright (c) ZeroC, Inc.

rootProject.name = "build-logic"

pluginManagement {
    repositories {
        mavenCentral()
        gradlePluginPortal()
    }
}

dependencyResolutionManagement {
    repositories {
        mavenCentral()
        gradlePluginPortal()
    }
}

// Include slice-tools so it's available for convention plugins
includeBuild("../tools/slice-tools") {
    dependencySubstitution {
        substitute(module("com.zeroc.slice:slice-tools")).using(project(":"))
    }
}
