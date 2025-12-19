// Copyright (c) ZeroC, Inc.

plugins {
    id("ice.application-conventions")
}

// Don't generate javadoc
tasks.named<Javadoc>("javadoc") {
    isEnabled = false
}

dependencies {
    "implementation"(project(":ice"))
}

tasks.named<Jar>("jar") {
    archiveFileName.set("IceTestPlugins.jar")
}
