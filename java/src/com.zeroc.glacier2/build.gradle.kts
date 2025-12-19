// Copyright (c) ZeroC, Inc.

val displayName by extra("Glacier2")
val moduleName by extra("com.zeroc.glacier2")
val projectDescription by extra("Firewall traversal for Ice")

val topSrcDir: String by project.extra

sourceSets {
    main {
        extensions.configure<org.gradle.api.file.SourceDirectorySet>("slice") {
            srcDir("$topSrcDir/slice/Glacier2")
            exclude("**/Metrics.ice")
        }
    }
}

dependencies {
    implementation(project(":ice"))
}

apply(from = "$topSrcDir/java/gradle/library.gradle.kts")
