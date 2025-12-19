// Copyright (c) ZeroC, Inc.

val runningInCi = providers.environmentVariable("CI").isPresent

apply(plugin = "checkstyle")
apply(plugin = "org.openrewrite.rewrite")

val topSrcDir: String by project.extra

// Configure the slice extension - it's available because slice-tools plugin is applied before this script
run {
    // Access the slice extension dynamically
    val sliceExt = extensions.getByName("slice")
    // Use reflection to set the properties
    sliceExt.javaClass.getMethod("getIncludeSearchPath").invoke(sliceExt).let { includeSearchPath ->
        (includeSearchPath as org.gradle.api.file.ConfigurableFileCollection).setFrom(files("$topSrcDir/slice"))
    }
    val defaultIceToolsPath = "${file(topSrcDir).canonicalPath}/cpp/bin"

    @Suppress("UNCHECKED_CAST")
    val toolsPathProperty = sliceExt.javaClass.getMethod("getToolsPath").invoke(sliceExt) as org.gradle.api.provider.Property<String>
    if (org.gradle.internal.os.OperatingSystem.current().isWindows) {
        val platform = project.findProperty("cppPlatform")?.toString() ?: "x64"
        val configuration = project.findProperty("cppConfiguration")?.toString() ?: "Release"
        toolsPathProperty.set("$defaultIceToolsPath/$platform/$configuration")
    } else {
        toolsPathProperty.set(defaultIceToolsPath)
    }
}

configure<CheckstyleExtension> {
    toolVersion = "10.21.4"

    // If we're running in CI, we want the build to fail if any warnings are emitted.
    // This doesn't affect what checkstyle emits, just whether it 'fails' or 'succeeds' from Gradle's perspective.
    if (runningInCi) {
        maxWarnings = 0
    }
}

tasks.withType<Checkstyle>().configureEach {
    // Ensure that running checkstyle multiple times always yields the same output. Otherwise, after an initial run,
    // the checkstyle task will be marked "UP TO DATE", and subsequent runs will skip over it.
    outputs.upToDateWhen { false }
    doNotTrackState("Always run Checkstyle")
}

val libDir by project.extra { "${rootProject.projectDir}/lib" }
