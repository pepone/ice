// Copyright (c) ZeroC, Inc.

plugins {
    id("ice.library-conventions")
}

iceLibrary {
    displayName.set("IceStorm")
    moduleName.set("com.zeroc.icestorm")
    projectDescription.set("Publish-subscribe event distribution service")
}

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
