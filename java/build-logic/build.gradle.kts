// Copyright (c) ZeroC, Inc.

plugins {
    `kotlin-dsl`
}

dependencies {
    implementation("com.zeroc:slice-tools")
}

gradlePlugin {
    plugins {
        register("ice-base") {
            id = "com.zeroc.ice-base"
            implementationClass = "com.zeroc.gradle.IceBasePlugin"
        }
        register("ice-library") {
            id = "com.zeroc.ice-library"
            implementationClass = "com.zeroc.gradle.IceLibraryPlugin"
        }
        register("ice-slice") {
            id = "com.zeroc.ice-slice"
            implementationClass = "com.zeroc.gradle.IceSlicePlugin"
        }
    }
}
