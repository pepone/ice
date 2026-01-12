// Copyright (c) ZeroC, Inc.

import proguard.gradle.ProGuardTask

buildscript {
    repositories {
        mavenCentral()
    }
    dependencies {
        classpath("com.guardsquare:proguard-gradle:7.6.0")
    }
}

plugins {
    id("com.zeroc.ice-base")
    // Note: IceGridGUI doesn't have Slice files, so we don't apply ice-slice
}

// Determine platform for JavaFX dependencies
val platform: String = when {
    System.getProperty("os.name") == "Mac OS X" -> "mac"
    System.getProperty("os.name").lowercase().contains("windows") -> "win"
    else -> "linux"
}

val topSrcDir: String by project.extra
val libDir = rootProject.projectDir.resolve("lib")

val jgoodiesLooksVersion: String by project
val jgoodiesFormsVersion: String by project
val openjfxVersion: String by project

// Don't generate Javadoc for this module
tasks.named<Javadoc>("javadoc") {
    isEnabled = false
}

// Disable deprecation warnings caused by JGoodies
tasks.withType<JavaCompile>().configureEach {
    options.isDeprecation = false
}

dependencies {
    implementation(project(":ice"))
    implementation(project(":icelocatordiscovery"))
    implementation(project(":icebox"))
    implementation(project(":icestorm"))
    implementation(project(":glacier2"))
    implementation(project(":icegrid"))

    // JGoodies UI libraries
    implementation("com.jgoodies:jgoodies-looks:$jgoodiesLooksVersion")
    implementation("com.jgoodies:jgoodies-forms:$jgoodiesFormsVersion")

    // JavaFX (platform-specific)
    implementation("org.openjfx:javafx-base:$openjfxVersion:$platform")
    implementation("org.openjfx:javafx-swing:$openjfxVersion:$platform")
    implementation("org.openjfx:javafx-controls:$openjfxVersion:$platform")
    implementation("org.openjfx:javafx-graphics:$openjfxVersion:$platform")
}

val tmpJarName = "IceGridGUITEMP.jar"
val jarName = "icegridgui.jar"

tasks.named<Jar>("jar") {
    archiveFileName.set(tmpJarName)
    manifest {
        attributes(
            "Main-Class" to "com.zeroc.IceGridGUI.Main",
            "Built-By" to "ZeroC, Inc."
        )
    }
}

// Library JARs for ProGuard
val libJars: List<String> = listOf(
    "java.base.jmod",
    "java.xml.jmod",
    "java.desktop.jmod",
    "java.prefs.jmod",
    "java.naming.jmod",
    "java.datatransfer.jmod",
    "jdk.unsupported.desktop.jmod",
    "javafx.base.jmod",
    "javafx.controls.jmod",
    "javafx.graphics.jmod",
    "javafx.swing.jmod",
    "java.logging.jmod"
).map { "${System.getProperty("java.home")}/jmods/$it" }

// ProGuard task
tasks.register<ProGuardTask>("proguardJar") {
    dependsOn(tasks.named("jar"))
    // Explicitly depend on dependency project jars
    dependsOn(":ice:jar")
    dependsOn(":glacier2:jar")
    dependsOn(":icebox:jar")
    dependsOn(":icegrid:jar")
    dependsOn(":icestorm:jar")
    dependsOn(":icelocatordiscovery:jar")

    // Add runtime classpath JARs with filter to exclude META-INF
    // The filter argument is a named parameter in Groovy: injars(files, filter: "...")
    // In Kotlin, we need to pass it as the first Map argument
    @Suppress("UNCHECKED_CAST")
    val filterArgs = mapOf("filter" to "!META-INF/**") as Map<Any?, Any?>
    injars(filterArgs, configurations.runtimeClasspath.get().resolve())
    injars(file("$projectDir/build/libs/$tmpJarName"))
    outjars(file("$libDir/$jarName"))
    libraryjars(libJars)
    configuration("icegridgui.pro")
}

tasks.named("assemble") {
    dependsOn("proguardJar")
}

tasks.named<Delete>("clean") {
    delete("$libDir/$jarName")
    delete("$libDir/IceGrid GUI.app")
}
