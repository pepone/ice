// Copyright (c) ZeroC, Inc.

// Note: buildscript blocks cannot access version catalogs, so version is hardcoded here
buildscript {
    repositories {
        mavenCentral()
    }

    dependencies {
        classpath("com.guardsquare:proguard-gradle:7.6.0")
    }
}

val tmpJarName: String by project.extra
val jarName: String by project.extra
val libJars: MutableList<String> by project.extra

listOf(
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
).forEach {
    libJars.add("${System.getProperty("java.home")}/jmods/$it")
}

tasks.register<proguard.gradle.ProGuardTask>("proguardJar") {
    dependsOn(tasks.named("jar"))
    // Add runtime classpath JARs with filter to exclude META-INF to avoid duplicates
    // Use Java HashMap to ensure proper type handling by ProGuard plugin
    val filterMap = java.util.HashMap<String, Any>()
    filterMap["filter"] = "!META-INF/**"
    injars(filterMap, configurations.getByName("runtimeClasspath"))
    injars(file("$projectDir/build/libs/$tmpJarName"))
    outjars(layout.buildDirectory.file("libs/$jarName"))
    libraryjars(libJars)
    configuration("icegridgui.pro")
}

tasks.named("assemble") {
    dependsOn(tasks.named("proguardJar"))
}
