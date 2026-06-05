// ===========================================================================
// 根 settings.gradle.kts (空, 由 app 子模块定义)
// ===========================================================================
pluginManagement {
    repositories {
        gradlePluginPortal()
        google()
        mavenCentral()
    }
}

dependencyResolutionManagement {
    repositoriesMode.set(RepositoriesMode.PREFER_SETTINGS)
    repositories {
        google()
        mavenCentral()
        gradlePluginPortal()
    }
}

rootProject.name = "VMCR"
include(":app")
