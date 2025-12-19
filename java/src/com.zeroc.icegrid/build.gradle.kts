// Copyright (c) ZeroC, Inc.

val displayName by extra("IceGrid")
val moduleName by extra("com.zeroc.icegrid")
val projectDescription by extra("Locate, deploy, and manage Ice servers")

val topSrcDir: String by project.extra

sourceSets {
    main {
        extensions.configure<org.gradle.api.file.SourceDirectorySet>("slice") {
            srcDir("$topSrcDir/slice/IceGrid")
        }
    }
}

dependencies {
    implementation(project(":ice"))
    implementation(project(":glacier2"))
}

apply(from = "$topSrcDir/java/gradle/library.gradle.kts")
