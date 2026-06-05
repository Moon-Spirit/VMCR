if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    add_library(vmcr_gles SHARED
        gles_renderer.cpp)

    target_compile_features(vmcr_gles PUBLIC cxx_std_20)
    target_link_libraries(vmcr_gles
        PRIVATE  vmcr_core
        PUBLIC   android log)

    set_source_files_properties(gles_renderer.cpp PROPERTIES
        COMPILE_DEFINITIONS VMCR_EXPORTS=1)
else()
    # 主机侧: 仅 stub, 不调用 vendor GL
    add_library(vmcr_gles STATIC
        gles_renderer_stub.cpp)
endif()