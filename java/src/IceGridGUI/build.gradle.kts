// Copyright (c) ZeroC, Inc.

plugins {
    id("ice.application-conventions")
}

val displayName by extra("IceGridGUI")
val projectDescription by extra("")

val jgoodiesLooksVersion: String by rootProject.extra
val jgoodiesFormsVersion: String by rootProject.extra
val openjfxVersion: String by rootProject.extra

val os: String = System.getProperty("os.name")
val platform: String = when {
    os == "Mac OS X" -> "mac"
    os.lowercase().contains("windows") -> "win"
    else -> "linux"
}

// Don't generate javadoc
tasks.named<Javadoc>("javadoc") {
    isEnabled = false
}

// Disable deprecation warnings caused by JGoodies
tasks.withType<JavaCompile> {
    options.isDeprecation = false
}

dependencies {
    "implementation"(project(":ice"))
    "implementation"(project(":icelocatordiscovery"))
    "implementation"(project(":icebox"))
    "implementation"(project(":icestorm"))
    "implementation"(project(":glacier2"))
    "implementation"(project(":icegrid"))
    "implementation"("com.jgoodies:jgoodies-looks:$jgoodiesLooksVersion")
    "implementation"("com.jgoodies:jgoodies-forms:$jgoodiesFormsVersion")

    "implementation"("org.openjfx:javafx-base:$openjfxVersion:$platform")
    "implementation"("org.openjfx:javafx-swing:$openjfxVersion:$platform")
    "implementation"("org.openjfx:javafx-controls:$openjfxVersion:$platform")
    "implementation"("org.openjfx:javafx-graphics:$openjfxVersion:$platform")
}

val tmpJarName by extra("IceGridGUITEMP.jar")
val jarName by extra("icegridgui.jar")

tasks.named<Jar>("jar") {
    archiveFileName.set(tmpJarName)
    manifest {
        attributes(
            "Main-Class" to "com.zeroc.IceGridGUI.Main",
            "Built-By" to "ZeroC, Inc."
        )
    }
}

val libJars by extra { mutableListOf<String>() }

apply(from = "proguard-jar.gradle.kts")
