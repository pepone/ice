// Copyright (c) ZeroC, Inc.

// Don't generate javadoc
tasks.named<Javadoc>("javadoc") {
    isEnabled = false
}

dependencies {
    "implementation"(project(":ice"))
}

tasks.named<Jar>("jar") {
    archiveFileName.set("IceTestPlugins.jar")
    destinationDirectory.set(file("${rootProject.projectDir}/lib/"))
}

tasks.named<Delete>("clean") {
    delete("${rootProject.projectDir}/lib/IceTestPlugins.jar")
}
