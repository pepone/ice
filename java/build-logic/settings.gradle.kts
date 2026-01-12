// Copyright (c) ZeroC, Inc.

rootProject.name = "build-logic"

pluginManagement {
    repositories {
        gradlePluginPortal()
    }
    includeBuild("../tools/slice-tools")
}

dependencyResolutionManagement {
    repositories {
        mavenCentral()
    }

    versionCatalogs {
        create("libs") {
            from(files("../gradle/libs.versions.toml"))
        }
    }
}
