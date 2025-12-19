// Copyright (c) ZeroC, Inc.

plugins {
    id("ice.library-conventions")
}

iceLibrary {
    displayName.set("IceLocatorDiscovery")
    moduleName.set("com.zeroc.icelocatordiscovery")
    projectDescription.set("Ice plug-in that enables the discovery of IceGrid and custom locators via UDP multicast")
}

val topSrcDir: String by project.extra

sourceSets {
    main {
        extensions.configure<org.gradle.api.file.SourceDirectorySet>("slice") {
            srcDir("$topSrcDir/slice/IceLocatorDiscovery")
        }
    }
}

dependencies {
    implementation(project(":ice"))
}
