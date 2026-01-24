option(peregrine_ENABLE_LOGGING "Enable logging is OFF" OFF)
if(peregrine_ENABLE_LOGGING)
    add_definitions(-Dperegrine_ENABLE_LOGGING=1)
endif()
option(peregrine_CUDA_ON "Build CUDA related code, disable other device backends. Default is OFF" OFF)
if(peregrine_CUDA_ON)
    add_definitions(-Dperegrine_CUDA_ON=1)
endif()



