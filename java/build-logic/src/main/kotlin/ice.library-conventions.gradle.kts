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
import org.gradle.api.artifacts.component.ProjectComponentIdentifier

plugins {
    id("ice.java-conventions")
    `java-library`
    `maven-publish`
}

val iceVersion = rootProject.findProperty("iceVersion")?.toString() ?: project.version.toString()

// Create the extension for subprojects to configure
val iceLibrary = extensions.create<IceLibraryExtension>("iceLibrary")

// Configure default values
iceLibrary.displayName.convention(project.name.replaceFirstChar { it.uppercase() })
iceLibrary.moduleName.convention("com.zeroc.${project.name}")
iceLibrary.projectDescription.convention("Ice ${project.name} module")

// Export extension properties for cross-project Javadoc linking
extra["iceLibraryDisplayName"] = iceLibrary.displayName
extra["iceLibraryModuleName"] = iceLibrary.moduleName

// Configure Javadoc with lazy configuration
val javadoc by tasks.existing(Javadoc::class) {
    dependsOn(tasks.named("compileSlice"))
    source(sourceSets["main"].allJava)
    destinationDirectory.set(layout.buildDirectory.dir("docs/javadoc"))
    isFailOnError = true
    (options as StandardJavadocDocletOptions).apply {
        addStringOption("Xdoclint:none", "-quiet")
        addBooleanOption("html5", true)
        header = iceLibrary.displayName.get()
        docTitle = "${iceLibrary.displayName.get()} $iceVersion API Reference"
    }

    // Use doFirst to lazily resolve dependencies and add linksOffline
    doFirst {
        configurations.getByName("compileClasspath").incoming.artifacts.artifacts.forEach { artifact ->
            val componentId = artifact.id.componentIdentifier
            if (componentId is ProjectComponentIdentifier) {
                val artifactProject = project(componentId.projectPath)
                val artifactDisplayName = (artifactProject.extra["iceLibraryDisplayName"] as Provider<String>).get()
                val artifactModuleName = (artifactProject.extra["iceLibraryModuleName"] as Provider<String>).get()

                (options as StandardJavadocDocletOptions).linksOffline(
                    "https://code.zeroc.com/ice/main/api/java/$artifactDisplayName/",
                    rootProject.projectDir.resolve("src/$artifactModuleName/build/docs/javadoc").absolutePath
                )
            }
        }
    }
}

// Ensure javadoc depends on dependency project javadocs
afterEvaluate {
    configurations.getByName("compileClasspath").incoming.afterResolve {
        artifacts.artifacts.forEach { artifact ->
            val componentId = artifact.id.componentIdentifier
            if (componentId is ProjectComponentIdentifier) {
                tasks.named("javadoc") {
                    dependsOn(project(componentId.projectPath).tasks.named("javadoc"))
                }
            }
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
                name.set(iceLibrary.displayName)
                description.set(iceLibrary.projectDescription)
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
    destination = layout.buildDirectory.file("publications/maven/pom-default.xml").get().asFile
}

// Configure assemble task dependencies
tasks.named("assemble") {
    dependsOn(tasks.named("jar"), tasks.named("sourcesJar"), tasks.named("javadocJar"))
    dependsOn(tasks.named("generatePomFileForMavenPublication"))
}
