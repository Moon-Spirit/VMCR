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

rootProject.name = "VMCR"
include(":app")
