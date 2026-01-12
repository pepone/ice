// Copyright (c) ZeroC, Inc.

import com.zeroc.slice.tools.slice

plugins {
    id("com.zeroc.ice-library")
    id("com.zeroc.ice-slice")
}

iceLibrary {
    displayName.set("Glacier2")
    moduleName.set("com.zeroc.glacier2")
    description.set("Firewall traversal for Ice")
}

val topSrcDir: String by project.extra

// Configure Slice source set
sourceSets {
    main {
        slice {
            srcDir("$topSrcDir/slice/Glacier2")
            setExcludes(listOf("**/Metrics.ice"))
        }
    }
}

dependencies {
    implementation(project(":ice"))
}
