// Copyright (c) ZeroC, Inc.

import com.zeroc.slice.tools.slice

plugins {
    id("com.zeroc.ice-library")
    id("com.zeroc.ice-slice")
}

iceLibrary {
    displayName.set("IceStorm")
    moduleName.set("com.zeroc.icestorm")
    description.set("Publish-subscribe event distribution service")
}

val topSrcDir: String by project.extra

sourceSets {
    main {
        slice {
            srcDir("$topSrcDir/slice/IceStorm")
            setExcludes(listOf("**/Metrics.ice"))
        }
    }
}

dependencies {
    implementation(project(":ice"))
}
