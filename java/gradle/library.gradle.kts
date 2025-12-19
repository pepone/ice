// Copyright (c) ZeroC, Inc.

// Note: maven-publish plugin is applied in the root build.gradle.kts subprojects block

val iceVersion: String by rootProject.extra
val libDir: String by project.extra
val displayName: String by project.extra
val moduleName: String by project.extra
val projectDescription: String by project.extra

tasks.named<Jar>("jar") {
    destinationDirectory.set(file(libDir))
}

tasks.named<Jar>("javadocJar") {
    destinationDirectory.set(file(libDir))
}

tasks.named<Jar>("sourcesJar") {
    destinationDirectory.set(file(libDir))
}

val compileSlice = tasks.named("compileSlice")

val sourceSets = the<SourceSetContainer>()

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

val pomName by project.extra { "$libDir/${project.name}-${project.version}.pom" }

apply(from = "${project.extra["topSrcDir"]}/java/gradle/maven-publish.gradle.kts")

tasks.named("assemble") {
    dependsOn(tasks.named("jar"), tasks.named("sourcesJar"), tasks.named("javadocJar"))
}

tasks.named<Jar>("jar") {
    manifest {
        attributes(
            "Class-Path" to configurations.getByName("runtimeClasspath").resolve().joinToString(" ") { it.name }
        )
    }
}

if (project.name == "icebox") {
    tasks.named<Jar>("jar") {
        manifest {
            attributes("Main-Class" to "com.zeroc.IceBox.Server")
        }
    }
}

tasks.named<Delete>("clean") {
    delete("$libDir/${project.name}-${project.version}.jar")
    delete("$libDir/${project.name}-${project.version}-sources.jar")
    delete("$libDir/${project.name}-${project.version}-javadoc.jar")
    delete(pomName)
}
