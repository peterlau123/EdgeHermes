option(edgehermes_ENABLE_LOGGING "Enable logging is OFF" OFF)
if(edgehermes_ENABLE_LOGGING)
    add_definitions(-Dedgehermes_ENABLE_LOGGING=1)
endif()
option(edgehermes_CUDA_ON "Build CUDA related code, disable other device backends. Default is OFF" OFF)
if(edgehermes_CUDA_ON)
    add_definitions(-Dedgehermes_CUDA_ON=1)
endif()


