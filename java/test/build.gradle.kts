// Copyright (c) ZeroC, Inc.

import com.zeroc.slice.tools.slice

plugins {
    id("com.zeroc.ice-base")
    id("com.zeroc.ice-slice")
}

val testDir = "$projectDir/src/main/java/test"
val libDir = rootProject.projectDir.resolve("lib")

// Don't generate Javadoc
tasks.named<Javadoc>("javadoc") {
    isEnabled = false
}

sourceSets {
    main {
        java {
            exclude("plugins")
        }

        slice {
            srcDir(testDir)
        }
    }
}

dependencies {
    implementation(project(":ice"))
    runtimeOnly(project(":icediscovery"))
    implementation(project(":icelocatordiscovery"))
    implementation(project(":icebox"))
    implementation(project(":glacier2"))
    implementation(project(":icestorm"))
    implementation(project(":icegrid"))
    implementation(project(":testPlugins"))
}

// Add commons-compress only when not offline
if (!gradle.startParameter.isOffline) {
    dependencies {
        runtimeOnly("org.apache.commons:commons-compress:1.20")
    }
}

tasks.named<Jar>("jar") {
    archiveFileName.set("test.jar")
    destinationDirectory.set(libDir)

    manifest {
        val classpath = configurations.runtimeClasspath.get()
            .resolve()
            .joinToString(" ") { it.toURI().toString() }
        attributes("Class-Path" to classpath)
    }
}

tasks.named<Delete>("clean") {
    delete("$libDir/test.jar")
    delete(fileTree("src/main/java/test/IceGrid/simple/db"))
    delete("src/main/java/test/Slice/generation/classes")
}
