// Copyright (c) ZeroC, Inc.

import com.zeroc.slice.tools.slice

plugins {
    id("com.zeroc.ice-library")
    id("com.zeroc.ice-slice")
}

iceLibrary {
    displayName.set("IceBT")
    moduleName.set("com.zeroc.icebt")
    description.set("Bluetooth support for Ice")
}

val topSrcDir: String by project.extra

sourceSets {
    main {
        slice {
            srcDir("$topSrcDir/slice/IceBT")
        }
    }
}

dependencies {
    implementation(project(":ice"))
}

tasks.named<Jar>("jar") {
    // The classes in src/main/java/android/bluetooth are stubs that allow us to compile
    // the IceBT transport plug-in without requiring an Android SDK.
    // These classes are excluded from the IceBT JAR file.
    exclude("android/**")
}

tasks.named<Javadoc>("javadoc") {
    exclude("**/android/*", "**/*I.java", "**/Instance.java")
}
