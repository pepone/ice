// Copyright (c) ZeroC, Inc.

import com.zeroc.slice.tools.slice

plugins {
    id("com.zeroc.ice-library")
    id("com.zeroc.ice-slice")
}

iceLibrary {
    displayName.set("IceBox")
    moduleName.set("com.zeroc.icebox")
    description.set("IceBox is an easy-to-use framework for Ice application services")
}

val topSrcDir: String by project.extra

sourceSets {
    main {
        slice {
            srcDir("$topSrcDir/slice/IceBox")
        }
    }
}

dependencies {
    implementation(project(":ice"))
}

tasks.named<Jar>("jar") {
    manifest {
        attributes("Main-Class" to "com.zeroc.IceBox.Server")
    }
}

tasks.named<Javadoc>("javadoc") {
    exclude("**/Admin.java", "**/Server.java", "**/ServiceManagerI.java")
}
