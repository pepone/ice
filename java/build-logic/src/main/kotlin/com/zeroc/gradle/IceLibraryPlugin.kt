// Copyright (c) ZeroC, Inc.

package com.zeroc.gradle

import org.gradle.api.Plugin
import org.gradle.api.Project
import org.gradle.api.plugins.JavaPluginExtension
import org.gradle.api.publish.PublishingExtension
import org.gradle.api.publish.maven.MavenPublication
import org.gradle.api.tasks.bundling.Jar
import org.gradle.kotlin.dsl.*

/**
 * Plugin for Ice library projects that will be published.
 * Extends the base plugin with publishing configuration.
 */
class IceLibraryPlugin : Plugin<Project> {
    override fun apply(project: Project) {
        with(project) {
            pluginManager.apply(IceBasePlugin::class.java)
            pluginManager.apply("maven-publish")

            // Create the iceLibrary extension
            val iceLibrary = extensions.create("iceLibrary", IceLibraryExtension::class.java)

            // Configure jar manifest
            afterEvaluate {
                tasks.withType(Jar::class.java).configureEach {
                    manifest {
                        attributes(mapOf(
                            "Built-By" to "ZeroC, Inc.",
                            "Implementation-Vendor" to "ZeroC, Inc.",
                            "Implementation-Title" to iceLibrary.displayName.getOrElse(project.name),
                            "Implementation-Version" to project.version
                        ))

                        // Add module name if specified
                        iceLibrary.moduleName.orNull?.let {
                            attributes(mapOf("Automatic-Module-Name" to it))
                        }
                    }
                }
            }

            // Configure Java extension to include sources jar
            extensions.configure<JavaPluginExtension>("java") {
                withSourcesJar()
            }

            // Configure publishing
            extensions.configure<PublishingExtension>("publishing") {
                publications {
                    create("maven", MavenPublication::class.java) {
                        from(components.getByName("java"))

                        pom {
                            name.convention(iceLibrary.displayName)
                            description.convention(iceLibrary.description)
                            url.set("https://zeroc.com")
                            organization {
                                name.set("ZeroC, Inc.")
                                url.set("https://zeroc.com")
                            }
                            licenses {
                                license {
                                    name.set("BSD-3-Clause")
                                    url.set("https://opensource.org/licenses/BSD-3-Clause")
                                    distribution.set("repo")
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
                                url.set("https://github.com/zeroc-ice/ice")
                                connection.set("scm:git:https://github.com/zeroc-ice/ice.git")
                                developerConnection.set("scm:git:https://github.com/zeroc-ice/ice.git")
                            }
                        }
                    }
                }
            }
        }
    }
}
