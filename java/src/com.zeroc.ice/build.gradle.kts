// Copyright (c) ZeroC, Inc.

import com.zeroc.slice.tools.slice

plugins {
    id("com.zeroc.ice-library")
    id("com.zeroc.ice-slice")
}

iceLibrary {
    displayName.set("Ice")
    moduleName.set("com.zeroc.ice")
    description.set(
        "Ice is a comprehensive RPC framework that helps you build distributed applications " +
        "with minimal effort using familiar object-oriented idioms"
    )
}

val topSrcDir: String by project.extra

sourceSets {
    main {
        slice {
            srcDir("$topSrcDir/slice/")
            // Use setIncludes to replace default patterns (like Groovy's includes = [...])
            setIncludes(listOf(
                "Ice/*.ice",
                "Glacier2/Metrics.ice",
                "IceStorm/Metrics.ice"
            ))
        }
    }
}
