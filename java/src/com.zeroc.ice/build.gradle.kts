// Copyright (c) ZeroC, Inc.

val displayName by extra("Ice")
val moduleName by extra("com.zeroc.ice")
val projectDescription by extra(
    "Ice is a comprehensive RPC framework that helps you build distributed applications" +
        " with minimal effort using familiar object-oriented idioms"
)

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

apply(from = "$topSrcDir/java/gradle/library.gradle.kts")
