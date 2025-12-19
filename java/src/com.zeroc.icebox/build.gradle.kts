// Copyright (c) ZeroC, Inc.

val displayName by extra("IceBox")
val moduleName by extra("com.zeroc.icebox")
val projectDescription by extra("IceBox is an easy-to-use framework for Ice application services")

val topSrcDir: String by project.extra

sourceSets {
    main {
        extensions.configure<org.gradle.api.file.SourceDirectorySet>("slice") {
            srcDir("$topSrcDir/slice/IceBox")
        }
    }
}

dependencies {
    implementation(project(":ice"))
}

apply(from = "$topSrcDir/java/gradle/library.gradle.kts")

tasks.named<Javadoc>("javadoc") {
    exclude("**/Admin.java", "**/Server.java", "**/ServiceManagerI.java")
}
