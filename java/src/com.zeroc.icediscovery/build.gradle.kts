// Copyright (c) ZeroC, Inc.

val displayName by extra("IceDiscovery")
val moduleName by extra("com.zeroc.icediscovery")
val projectDescription by extra("Allow Ice applications to discover objects and object adapters")

val topSrcDir: String by project.extra

sourceSets {
    main {
        extensions.configure<org.gradle.api.file.SourceDirectorySet>("slice") {
            srcDir("$topSrcDir/slice/IceDiscovery")
        }
    }
}

dependencies {
    implementation(project(":ice"))
}

apply(from = "$topSrcDir/java/gradle/library.gradle.kts")
