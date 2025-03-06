package com.zeroc

import org.gradle.api.Project
import org.gradle.api.plugins.ExtensionAware
import org.gradle.api.plugins.JavaPluginExtension
import org.gradle.api.tasks.TaskProvider
import org.gradle.api.Task
import org.gradle.api.GradleException

object SliceToolsJava {
    fun configure(project: Project, extension: SliceExtension, compileSlice: TaskProvider<Task>) {
        val javaExtension = project.extensions.findByType(JavaPluginExtension::class.java)
            ?: throw GradleException("JavaPluginExtension is missing. Ensure the Java plugin is applied before configuring Slice Tools.")

        javaExtension.sourceSets.configureEach { sourceSet ->
            val sliceSourceSet = extension.sourceSets.create(sourceSet.name)
            SliceToolsUtil.addSourceSetExtension(project, sourceSet.name, sourceSet as ExtensionAware, sliceSourceSet)
            SliceToolsUtil.configureSliceTaskForSourceSet(project, extension, sliceSourceSet, compileSlice)
            sourceSet.java.srcDirs(sliceSourceSet.output)
        }
    }
}
