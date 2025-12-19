// Copyright (c) ZeroC, Inc.

plugins {
    id("ice.library-conventions")
}

iceLibrary {
    displayName.set("Ice")
    moduleName.set("com.zeroc.ice")
    projectDescription.set(
        "Ice is a comprehensive RPC framework that helps you build distributed applications" +
            " with minimal effort using familiar object-oriented idioms"
    )
}

val topSrcDir: String by project.extra

sourceSets {
    main {
        extensions.configure<org.gradle.api.file.SourceDirectorySet>("slice") {
            srcDir("$topSrcDir/slice/")
            // Use setIncludes to replace the default "**/*.ice" pattern
            setIncludes(listOf("Ice/*.ice", "Glacier2/Metrics.ice", "IceStorm/Metrics.ice"))
        }
    }
}
