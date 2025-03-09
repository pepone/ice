/*
 * Gradle build file for the Slice Tools plugin.
 */

plugins {
    // Apply the Java Gradle Plugin Development plugin
    `java-gradle-plugin`
    `maven-publish`

    // Apply the Kotlin JVM plugin
    alias(libs.plugins.kotlin.jvm)

    // Code formatting with Spotless
    id("com.diffplug.spotless") version "6.25.0"
}

spotless {
    kotlin {
        target("src/**/*.kt") // Format all Kotlin files
        ktlint("0.50.0") // Use ktlint as the formatter
        trimTrailingWhitespace()
    }
}

repositories {
    google() // Required for AGP (Android Gradle Plugin)
    mavenCentral()
}

dependencies {
    compileOnly("com.android.tools.build:gradle:8.1.0")
    testImplementation("org.jetbrains.kotlin:kotlin-test-junit5")
    testRuntimeOnly("org.junit.platform:junit-platform-launcher")
}

// Plugin version management
val pluginVersion: String by project
version = pluginVersion

gradlePlugin {
    plugins {
        create("sliceToolsJava") {
            id = "com.zeroc.slice-tools-java"
            implementationClass = "com.zeroc.SliceToolsJavaPlugin"
        }
    }
}

// Publish to Maven repository
publishing {
    repositories {
        maven {
            name = "MyRepo"
            url = uri(findProperty("mavenRepository") as String? ?: System.getenv("MAVEN_REPOSITORY"))

            credentials {
                username = findProperty("mavenUsername") as String? ?: System.getenv("MAVEN_USERNAME")
                password = findProperty("mavenPassword") as String? ?: System.getenv("MAVEN_PASSWORD")
            }
        }
    }
}

// Configure the functional test source set
val functionalTestSourceSet = sourceSets.create("functionalTest")

configurations["functionalTestImplementation"].extendsFrom(configurations["testImplementation"])
configurations["functionalTestRuntimeOnly"].extendsFrom(configurations["testRuntimeOnly"])

// Register functional test task
val functionalTest by tasks.registering(Test::class) {
    testClassesDirs = functionalTestSourceSet.output.classesDirs
    classpath = functionalTestSourceSet.runtimeClasspath
    useJUnitPlatform()
}

gradlePlugin.testSourceSets.add(functionalTestSourceSet)

// Ensure functional tests run as part of the `check` task
tasks.named<Task>("check") {
    dependsOn(functionalTest)
}

// Configure unit tests to use JUnit Jupiter
tasks.named<Test>("test") {
    useJUnitPlatform()
}
