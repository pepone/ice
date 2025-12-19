// Copyright (c) ZeroC, Inc.

val displayName by extra("IceLocatorDiscovery")
val moduleName by extra("com.zeroc.icelocatordiscovery")
val projectDescription by extra("Ice plug-in that enables the discovery of IceGrid and custom locators via UDP multicast")

val topSrcDir: String by project.extra

sourceSets {
    main {
        extensions.configure<org.gradle.api.file.SourceDirectorySet>("slice") {
            srcDir("$topSrcDir/slice/IceLocatorDiscovery")
        }
    }
}

dependencies {
    implementation(project(":ice"))
}

apply(from = "$topSrcDir/java/gradle/library.gradle.kts")
