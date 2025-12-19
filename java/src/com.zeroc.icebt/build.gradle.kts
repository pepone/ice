// Copyright (c) ZeroC, Inc.

plugins {
    id("ice.library-conventions")
}

iceLibrary {
    displayName.set("IceBT")
    moduleName.set("com.zeroc.icebt")
    projectDescription.set("Bluetooth support for Ice")
}

val topSrcDir: String by project.extra

sourceSets {
    main {
        extensions.configure<org.gradle.api.file.SourceDirectorySet>("slice") {
            srcDir("$topSrcDir/slice/IceBT")
        }
    }
}

dependencies {
    implementation(project(":ice"))
}

tasks.named<Jar>("jar") {
    //
    // The classes in src/main/java/android/bluetooth are stubs that allow us to compile the IceBT transport
    // plug-in without requiring an Android SDK. These classes are excluded from the IceBT JAR file.
    //
    exclude("android/**")
}

tasks.named<Javadoc>("javadoc") {
    exclude("**/android/*", "**/*I.java", "**/Instance.java")
}
