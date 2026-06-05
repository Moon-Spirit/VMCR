if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    # 入口库 - libGL.so
    add_library(vmcr_loader SHARED
        gl_entry.cpp
        vmcr_init.cpp
        dlsym_vendor.cpp
        android_dlopen.cpp)

    target_compile_features(vmcr_loader PUBLIC cxx_std_20)
    target_include_directories(vmcr_loader PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(vmcr_loader
        PRIVATE  vmcr_core
        PUBLIC   android log)

    # 关键: 输出名必须是 libGL.so
    set_target_properties(vmcr_loader PROPERTIES
        OUTPUT_NAME "GL"
        PREFIX "lib"
        C_VISIBILITY_PRESET   default
        CXX_VISIBILITY_PRESET default)

    target_compile_definitions(vmcr_loader PRIVATE VMCR_EXPORTS_GLSYMS=1)

    # 入口库 - libEGL.so
    add_library(vmcr_egl SHARED
        egl_entry.cpp)

    target_compile_features(vmcr_egl PUBLIC cxx_std_20)
    target_include_directories(vmcr_egl PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(vmcr_egl
        PRIVATE  vmcr_core vmcr_loader
        PUBLIC   android log)

    set_target_properties(vmcr_egl PROPERTIES
        OUTPUT_NAME "EGL"
        PREFIX "lib"
        C_VISIBILITY_PRESET   default
        CXX_VISIBILITY_PRESET default)
else()
    # 主机侧: 空 stub (egl_entry 引用 vendor_egl, 主机无 vendor)
    add_library(vmcr_loader STATIC loader_stub.cpp)
    add_library(vmcr_egl INTERFACE)
endif()