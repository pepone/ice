// Copyright (c) ZeroC, Inc.

val displayName by extra("IceStorm")
val moduleName by extra("com.zeroc.icestorm")
val projectDescription by extra("Publish-subscribe event distribution service")

val topSrcDir: String by project.extra

sourceSets {
    main {
        extensions.configure<org.gradle.api.file.SourceDirectorySet>("slice") {
            srcDir("$topSrcDir/slice/IceStorm")
            exclude("**/Metrics.ice")
        }
    }
}

dependencies {
    implementation(project(":ice"))
}

apply(from = "$topSrcDir/java/gradle/library.gradle.kts")
