// Copyright (c) ZeroC, Inc.

import org.gradle.api.tasks.javadoc.Javadoc
import org.gradle.api.tasks.SourceSetContainer
import org.gradle.external.javadoc.StandardJavadocDocletOptions

plugins {
    id("com.zeroc.slice-tools") apply false
    alias(libs.plugins.openrewrite)
}

val iceVersion = libs.versions.ice.get()

// Clean task
tasks.register<Delete>("clean") {
    delete(rootProject.layout.buildDirectory)
}

// Distribution task - assembles all library JARs
val dist by tasks.registering {
    dependsOn(
        project(":ice").tasks.named("assemble"),
        project(":glacier2").tasks.named("assemble"),
        project(":icegrid").tasks.named("assemble"),
        project(":icebox").tasks.named("assemble"),
        project(":icebt").tasks.named("assemble"),
        project(":icediscovery").tasks.named("assemble"),
        project(":icelocatordiscovery").tasks.named("assemble"),
        project(":icestorm").tasks.named("assemble"),
        project(":IceGridGUI").tasks.named("assemble")
    )
}

// Make test compilation depend on dist
project(":test").afterEvaluate {
    tasks.named("compileJava") {
        dependsOn(dist)
    }
}

// Projects to include in aggregated Javadoc
val exportedProjects = listOf(
    ":glacier2",
    ":ice",
    ":icebox",
    ":icebt",
    ":icegrid",
    ":icestorm"
)

// Aggregated Javadoc task
tasks.register<Javadoc>("alljavadoc") {
    group = "documentation"
    description = "Generates aggregated Javadoc for all exported projects"

    // Add source files from all exported projects
    source = files(exportedProjects.map { project(it).sourceSets["main"].allJava }).asFileTree

    // Output directory
    destinationDir = layout.buildDirectory.dir("docs/javadoc").get().asFile

    (options as StandardJavadocDocletOptions).apply {
        encoding = "UTF-8"
        addStringOption(
            "-module-source-path",
            "./src/*/src/main/java/${File.pathSeparator}./src/*/build/generated/source/slice/main"
        )
        addBooleanOption("html5", true)
        header = "Ice for Java"
        docTitle = "Ice $iceVersion API Reference"
    }

    isFailOnError = true

    exclude(
        "**/android/bluetooth/*.java",
        "**/com/zeroc/Ice/CommunicatorObserverI.java",
        "**/com/zeroc/Ice/InvocationObserverI.java",
        "**/com/zeroc/Ice/ConnectionI.java"
    )

    // Depend on assemble tasks of exported projects
    dependsOn(exportedProjects.map { project(it).tasks.named("assemble") })
}

// OpenRewrite configuration
rewrite {
    activeRecipe("com.zeroc.IceRewriteRecipes")
    activeStyle("com.zeroc.IceRewriteStyle")

    setExportDatatables(true)
    exclusion(
        // OpenRewrite is technically capable of modifying Groovy and Kotlin files, but we avoid this
        "**/*.gradle",
        "**/*.kts",
        // Explicitly excluded file
        "java/src/com.zeroc.ice/src/main/java/com/zeroc/Ice/PropertyNames.java",
        // Generated files
        "**/generated/**/*",
        // Files with import shortener issues due to Ice object names
        "java/src/com.zeroc.ice/src/main/java/com/zeroc/Ice/Properties.java",
        "java/src/com.zeroc.ice/src/main/java/com/zeroc/Ice/InputStream.java",
        "java/src/IceGridGUI/src/main/java/com/zeroc/IceGridGUI/LiveDeployment/ShowIceLogDialog.java",
        "java/src/IceGridGUI/src/main/java/com/zeroc/IceGridGUI/LiveDeployment/ShowLogFileDialog.java",
        // File that OpenRewrite continuously attempts to reformat
        "java/test/src/main/java/test/Ice/operations/Twoways.java"
    )
}

dependencies {
    rewrite(libs.openrewrite.static.analysis)
    rewrite(libs.openrewrite.java.dependencies)
}

// Configure rewriteDryRun task
tasks.named("rewriteDryRun") {
    doFirst {
        // Delete old report file if present
        val reportFile = file("${layout.buildDirectory.get()}/reports/rewrite/rewrite.patch")
        if (reportFile.exists()) {
            reportFile.delete()
        }
    }

    doLast {
        // Fail CI if there are any changes
        val reportFile = file("${layout.buildDirectory.get()}/reports/rewrite/rewrite.patch")
        if (providers.environmentVariable("CI").isPresent && reportFile.exists() && reportFile.length() > 0) {
            throw GradleException(
                "OpenRewrite found issues that need to be fixed. " +
                "Run './gradlew rewriteRun' to apply fixes."
            )
        }
    }
}

// Extension to access sourceSets from other projects (used by alljavadoc task)
val Project.sourceSets: SourceSetContainer
    get() = extensions.getByType()
