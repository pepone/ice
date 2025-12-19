// Copyright (c) ZeroC, Inc.

/**
 * Convention plugin for Ice Java library modules.
 * Applies java-library plugin, configures Javadoc, jar outputs, and Maven publishing.
 *
 * Usage in subprojects:
 * ```
 * plugins {
 *     id("ice.library-conventions")
 * }
 *
 * iceLibrary {
 *     displayName.set("Glacier2")
 *     moduleName.set("com.zeroc.glacier2")
 *     projectDescription.set("Firewall traversal for Ice")
 * }
 * ```
 */

import com.zeroc.ice.gradle.IceLibraryExtension

plugins {
    id("ice.java-conventions")
    `java-library`
    `maven-publish`
}

val iceVersion = rootProject.findProperty("iceVersion")?.toString() ?: project.version.toString()
val libDir: String by project.extra

// Create the extension for subprojects to configure
val iceLibrary = extensions.create<IceLibraryExtension>("iceLibrary")

// Configure default values
iceLibrary.displayName.convention(project.name.replaceFirstChar { it.uppercase() })
iceLibrary.moduleName.convention("com.zeroc.${project.name}")
iceLibrary.projectDescription.convention("Ice ${project.name} module")

// Compute POM path
val pomName = "$libDir/${project.name}-${project.version}.pom"
extra["pomName"] = pomName

// Configure jar output directories
tasks.named<Jar>("jar") {
    destinationDirectory.set(file(libDir))
}

tasks.named<Jar>("javadocJar") {
    destinationDirectory.set(file(libDir))
}

tasks.named<Jar>("sourcesJar") {
    destinationDirectory.set(file(libDir))
}

// Configure Javadoc - use afterEvaluate to ensure extension values are available
afterEvaluate {
    val displayName = iceLibrary.displayName.get()
    val moduleName = iceLibrary.moduleName.get()

    // Export these as extra properties for cross-project Javadoc linking
    extra["displayName"] = displayName
    extra["moduleName"] = moduleName

    val compileSlice = tasks.named("compileSlice")
    val sourceSets = extensions.getByType<SourceSetContainer>()

    tasks.named<Javadoc>("javadoc") {
        dependsOn(compileSlice)
        source = sourceSets["main"].allJava
        doFirst {
            (options as StandardJavadocDocletOptions).addStringOption("Xdoclint:none", "-quiet")
        }
        isFailOnError = true
        (options as StandardJavadocDocletOptions).apply {
            header = displayName
            addBooleanOption("html5", true)
            docTitle = "$displayName $iceVersion API Reference"
        }
        destinationDir = file("${layout.buildDirectory.get()}/docs/javadoc")

        configurations.getByName("compileClasspath").resolvedConfiguration.resolvedArtifacts.forEach { artifact ->
            if (artifact.moduleVersion.id.group == "com.zeroc") {
                val artifactProject = project(":${artifact.name}")

                // Add dependency on the Javadoc task of the artifact project
                dependsOn(artifactProject.tasks.named("javadoc"))

                val artifactDisplayName = artifactProject.extra["displayName"] as String
                val artifactModuleName = artifactProject.extra["moduleName"] as String

                (options as StandardJavadocDocletOptions).linksOffline(
                    "https://code.zeroc.com/ice/main/api/java/$artifactDisplayName/",
                    "${rootProject.projectDir}/src/$artifactModuleName/build/docs/javadoc"
                )
            }
        }
    }

    // Configure Maven publishing
    configure<PublishingExtension> {
        publications {
            create<MavenPublication>("maven") {
                groupId = "com.zeroc"
                artifactId = project.name
                version = project.version.toString()

                from(components["java"])
                pom {
                    name.set(displayName)
                    description.set(iceLibrary.projectDescription.get())
                    url.set("https://zeroc.com")
                    licenses {
                        license {
                            name.set("GNU General Public License, version 2")
                            url.set("https://www.gnu.org/licenses/gpl-2.0.html")
                        }
                    }
                    developers {
                        developer {
                            name.set("ZeroC Developers")
                            email.set("info@zeroc.com")
                            organization.set("ZeroC, Inc.")
                            organizationUrl.set("https://zeroc.com")
                        }
                    }
                    scm {
                        connection.set("scm:git:git://github.com:zeroc-ice/ice.git")
                        url.set("https://github.com:zeroc-ice/ice")
                    }
                }
            }
        }
    }
}

// Configure jar manifest with classpath
tasks.named<Jar>("jar") {
    manifest {
        attributes(
            "Class-Path" to configurations.getByName("runtimeClasspath").resolve().joinToString(" ") { it.name }
        )
    }
}

// Special case for icebox - add Main-Class
if (project.name == "icebox") {
    tasks.named<Jar>("jar") {
        manifest {
            attributes("Main-Class" to "com.zeroc.IceBox.Server")
        }
    }
}

// Configure POM generation
tasks.withType<GenerateMavenPom>().configureEach {
    destination = file(pomName)
}

// Configure assemble task dependencies
tasks.named("assemble") {
    dependsOn(tasks.named("jar"), tasks.named("sourcesJar"), tasks.named("javadocJar"))
    dependsOn(tasks.named("generatePomFileForMavenPublication"))
}

// Configure clean task
tasks.named<Delete>("clean") {
    delete("$libDir/${project.name}-${project.version}.jar")
    delete("$libDir/${project.name}-${project.version}-sources.jar")
    delete("$libDir/${project.name}-${project.version}-javadoc.jar")
    delete(pomName)
}
