// Copyright (c) ZeroC, Inc.

val testDir = "${projectDir}/src/main/java/test"

// Don't generate javadoc
tasks.named<Javadoc>("javadoc") {
    isEnabled = false
}

configure<SourceSetContainer> {
    named("main") {
        java {
            exclude("plugins")
        }
    }
}

sourceSets {
    main {
        extensions.configure<org.gradle.api.file.SourceDirectorySet>("slice") {
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

if (!gradle.startParameter.isOffline) {
    dependencies {
        runtimeOnly("org.apache.commons:commons-compress:1.20")
    }
}

tasks.named<Jar>("jar") {
    archiveFileName.set("test.jar")
    manifest {
        attributes("Class-Path" to configurations.getByName("runtimeClasspath").resolve().joinToString(" ") { it.toURI().toString() })
    }
}

tasks.named<Delete>("clean") {
    listOf("src/main/java/test/IceGrid/simple/db").forEach {
        delete(fileTree(it))
    }
    delete("src/main/java/test/Slice/generation/classes")
}
