// Copyright (c) ZeroC, Inc.

plugins {
    id("com.zeroc.slice-tools") apply false
    id("checkstyle")
    id("org.openrewrite.rewrite") version "7.19.0"
}

val iceVersion: String by project
val targetJavaRelease: String by project
val debug: String by project

allprojects {
    repositories {
        mavenCentral()
    }
}

subprojects {
    extra["topSrcDir"] = "${rootProject.projectDir}/.."

    version = iceVersion
    group = "com.zeroc"

    apply(plugin = "checkstyle")
    apply(plugin = "java")
    apply(plugin = "com.zeroc.slice-tools")
    
    // Apply maven-publish only to library projects, not test projects
    if (name != "test" && name != "testPlugins") {
        apply(plugin = "maven-publish")
    }
    
    apply(from = "${rootProject.projectDir}/gradle/ice.gradle.kts")

    repositories {
        mavenCentral()
    }

    configure<JavaPluginExtension> {
        toolchain {
            languageVersion.set(JavaLanguageVersion.of(targetJavaRelease))
        }
        withSourcesJar()
        withJavadocJar()
    }

    tasks.withType<Jar> {
        manifest {
            attributes("Built-By" to "ZeroC, Inc.")
        }
    }

    tasks.withType<JavaCompile> {
        options.compilerArgs.addAll(
            listOf(
                "-Xdoclint:all,-missing",
                "-Xlint:all,-rawtypes,-exports,-serial,-try,-missing-explicit-ctor,-deprecation"
            )
        )
        options.encoding = "UTF-8"
        options.isDeprecation = true
    }
}

tasks.withType<JavaCompile>().configureEach {
    options.release.set(targetJavaRelease.toInt())
    options.isDebug = debug.toBoolean()
}

tasks.register<Delete>("clean") {
    delete(rootProject.layout.buildDirectory)
}

val dist by tasks.registering {
    dependsOn(project(":ice").tasks.named("assemble"))
    dependsOn(project(":glacier2").tasks.named("assemble"))
    dependsOn(project(":icegrid").tasks.named("assemble"))
    dependsOn(project(":icebox").tasks.named("assemble"))
    dependsOn(project(":icebt").tasks.named("assemble"))
    dependsOn(project(":icediscovery").tasks.named("assemble"))
    dependsOn(project(":icelocatordiscovery").tasks.named("assemble"))
    dependsOn(project(":icestorm").tasks.named("assemble"))
    dependsOn(project(":IceGridGUI").tasks.named("assemble"))
}

project(":test").tasks.named("compileJava") {
    dependsOn(dist)
}

val exportedProjects = listOf(
    ":glacier2",
    ":ice",
    ":icebox",
    ":icebt",
    ":icegrid",
    ":icestorm"
)

tasks.register<Javadoc>("alljavadoc") {
    // Add the source files from all subprojects
    source(exportedProjects.map { project(it).the<SourceSetContainer>()["main"].allJava })
    // Output directory for the aggregated Javadocs
    destinationDir = file("${layout.buildDirectory.get()}/docs/javadoc")
    options.encoding = "UTF-8"
    // Where to find source files for the different modules
    (options as StandardJavadocDocletOptions).apply {
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
}

tasks.named("alljavadoc") {
    dependsOn(exportedProjects.map { project(it).tasks.named("assemble") })
}

configure<CheckstyleExtension> {
    toolVersion = "10.21.4"
    isIgnoreFailures = false
    isShowViolations = true
}

configure<org.openrewrite.gradle.RewriteExtension> {
    activeRecipe("com.zeroc.IceRewriteRecipes")
    activeStyle("com.zeroc.IceRewriteStyle")

    setExportDatatables(true)
    exclusion(
        // OpenRewrite is technically capable of modifying Groovy and Kotlin files, but we are avoiding this for now.
        "**/*.gradle",
        "**/*.kts",
        // I have been explicitly instructed not to modify the below file.
        "java/src/com.zeroc.ice/src/main/java/com/zeroc/Ice/PropertyNames.java",
        // These are all generated files, and thus should not be subject to modification from OpenRewrite.
        "**/generated/**/*",
        // The following four files all have issues regarding the import shortener because of Ice object names.
        "java/src/com.zeroc.ice/src/main/java/com/zeroc/Ice/Properties.java",
        "java/src/com.zeroc.ice/src/main/java/com/zeroc/Ice/InputStream.java",
        "java/src/IceGridGUI/src/main/java/com/zeroc/IceGridGUI/LiveDeployment/ShowIceLogDialog.java",
        "java/src/IceGridGUI/src/main/java/com/zeroc/IceGridGUI/LiveDeployment/ShowLogFileDialog.java",
        // This file is never effectively fixed by OpenRewrite, yet OpenRewrite continually attempts to reformat it.
        "java/test/src/main/java/test/Ice/operations/Twoways.java"
    )
}

dependencies {
    "rewrite"("org.openrewrite.recipe:rewrite-static-analysis:2.19.0")
    "rewrite"("org.openrewrite.recipe:rewrite-java-dependencies:1.43.0")
}

tasks.named("rewriteDryRun") {
    val reportPath = project.layout.buildDirectory.file("reports/rewrite/rewrite.patch").get().asFile

    doFirst {
        // Delete the old report file if present
        if (reportPath.exists()) {
            reportPath.delete()
        }
    }

    doLast {
        if (reportPath.exists()) {
            print("\n\n" + java.nio.file.Files.readString(reportPath.toPath()))

            if (providers.environmentVariable("CI").isPresent) {
                throw RuntimeException("Applying recipes would make changes. See logs for more details.")
            }
        }
    }
}
