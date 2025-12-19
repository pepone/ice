// Copyright (c) ZeroC, Inc.

plugins {
    id("ice.library-conventions")
}

iceLibrary {
    displayName.set("Glacier2")
    moduleName.set("com.zeroc.glacier2")
    projectDescription.set("Firewall traversal for Ice")
}

val topSrcDir: String by project.extra

sourceSets {
    main {
        extensions.configure<org.gradle.api.file.SourceDirectorySet>("slice") {
            srcDir("$topSrcDir/slice/Glacier2")
            exclude("**/Metrics.ice")
        }
    }
}

dependencies {
    implementation(project(":ice"))
}
