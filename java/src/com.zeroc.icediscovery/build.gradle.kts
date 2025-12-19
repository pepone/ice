// Copyright (c) ZeroC, Inc.

plugins {
    id("ice.library-conventions")
}

iceLibrary {
    displayName.set("IceDiscovery")
    moduleName.set("com.zeroc.icediscovery")
    projectDescription.set("Allow Ice applications to discover objects and object adapters")
}

val topSrcDir: String by project.extra

sourceSets {
    main {
        extensions.configure<org.gradle.api.file.SourceDirectorySet>("slice") {
            srcDir("$topSrcDir/slice/IceDiscovery")
        }
    }
}

dependencies {
    implementation(project(":ice"))
}
