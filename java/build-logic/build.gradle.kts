// Copyright (c) ZeroC, Inc.

plugins {
    `kotlin-dsl`
}

repositories {
    mavenCentral()
    gradlePluginPortal()
}

dependencies {
    // Make slice-tools plugin available to convention plugins
    // This uses the included build from the parent project's settings.gradle.kts
    implementation("com.zeroc.slice:slice-tools")
}
