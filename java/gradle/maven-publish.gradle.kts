// Copyright (c) ZeroC, Inc.

// Note: maven-publish plugin must be applied in the subprojects block before this script is applied

val pomName: String by project.extra
val displayName: String by project.extra
val projectDescription: String by project.extra

tasks.withType<GenerateMavenPom>().configureEach {
    destination = file(pomName)
}

configure<PublishingExtension> {
    publications {
        create<MavenPublication>("maven") {
            groupId = "com.zeroc"
            artifactId = project.name
            version = project.version.toString()

            from(components["java"])
            pom {
                name.set(displayName)
                description.set(projectDescription)
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

tasks.named("assemble") {
    dependsOn(tasks.named("generatePomFileForMavenPublication"))
}
