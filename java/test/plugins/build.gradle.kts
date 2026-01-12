// Copyright (c) ZeroC, Inc.

plugins {
    id("com.zeroc.ice-base")
}

val libDir = rootProject.projectDir.resolve("lib")

// Don't generate Javadoc
tasks.named<Javadoc>("javadoc") {
    isEnabled = false
}

dependencies {
    implementation(project(":ice"))
}

tasks.named<Jar>("jar") {
    archiveFileName.set("IceTestPlugins.jar")
    destinationDirectory.set(libDir)
}

tasks.named<Delete>("clean") {
    delete("$libDir/IceTestPlugins.jar")
}
