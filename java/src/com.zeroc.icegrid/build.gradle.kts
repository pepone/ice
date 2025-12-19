// Copyright (c) ZeroC, Inc.

plugins {
    id("ice.library-conventions")
}

iceLibrary {
    displayName.set("IceGrid")
    moduleName.set("com.zeroc.icegrid")
    projectDescription.set("Locate, deploy, and manage Ice servers")
}

val topSrcDir: String by project.extra

sourceSets {
    main {
        extensions.configure<org.gradle.api.file.SourceDirectorySet>("slice") {
            srcDir("$topSrcDir/slice/IceGrid")
        }
    }
}

dependencies {
    implementation(project(":ice"))
    implementation(project(":glacier2"))
}
