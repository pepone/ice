// Copyright (c) ZeroC, Inc.

import com.zeroc.slice.tools.slice

plugins {
    id("com.zeroc.ice-library")
    id("com.zeroc.ice-slice")
}

iceLibrary {
    displayName.set("IceGrid")
    moduleName.set("com.zeroc.icegrid")
    description.set("Locate, deploy, and manage Ice servers")
}

val topSrcDir: String by project.extra

sourceSets {
    main {
        slice {
            srcDir("$topSrcDir/slice/IceGrid")
        }
    }
}

dependencies {
    implementation(project(":ice"))
    implementation(project(":glacier2"))
}
