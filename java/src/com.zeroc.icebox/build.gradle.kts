// Copyright (c) ZeroC, Inc.

plugins {
    id("ice.library-conventions")
}

iceLibrary {
    displayName.set("IceBox")
    moduleName.set("com.zeroc.icebox")
    projectDescription.set("IceBox is an easy-to-use framework for Ice application services")
}

val topSrcDir: String by project.extra

sourceSets {
    main {
        extensions.configure<org.gradle.api.file.SourceDirectorySet>("slice") {
            srcDir("$topSrcDir/slice/IceBox")
        }
    }
}

dependencies {
    implementation(project(":ice"))
}

tasks.named<Javadoc>("javadoc") {
    exclude("**/Admin.java", "**/Server.java", "**/ServiceManagerI.java")
}
