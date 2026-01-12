// Copyright (c) ZeroC, Inc.

import com.zeroc.slice.tools.slice

plugins {
    id("com.zeroc.ice-library")
    id("com.zeroc.ice-slice")
}

iceLibrary {
    displayName.set("IceDiscovery")
    moduleName.set("com.zeroc.icediscovery")
    description.set("Allow Ice applications to discover objects and object adapters")
}

val topSrcDir: String by project.extra

sourceSets {
    main {
        slice {
            srcDir("$topSrcDir/slice/IceDiscovery")
        }
    }
}

dependencies {
    implementation(project(":ice"))
}
