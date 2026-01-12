// Copyright (c) ZeroC, Inc.

import com.zeroc.slice.tools.slice

plugins {
    id("com.zeroc.ice-library")
    id("com.zeroc.ice-slice")
}

iceLibrary {
    displayName.set("IceLocatorDiscovery")
    moduleName.set("com.zeroc.icelocatordiscovery")
    description.set("Ice plug-in that enables the discovery of IceGrid and custom locators via UDP multicast")
}

val topSrcDir: String by project.extra

sourceSets {
    main {
        slice {
            srcDir("$topSrcDir/slice/IceLocatorDiscovery")
        }
    }
}

dependencies {
    implementation(project(":ice"))
}
