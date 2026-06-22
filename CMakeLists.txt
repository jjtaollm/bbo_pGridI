cmake_minimum_required(VERSION 3.10)
project(iono)



set(MKLROOT "/home/jtao//thirdparty/mkl")
set(MKLLIB "/home/jtao//thirdparty/mkl/lib")
set(MYSQL_DIR "/usr/lib64/mysql")
set(REDIS_DIR "/home/jtao//thirdparty/hiredis")
set(GSL_DIR "/home/jtao//thirdparty/gsl/")
set(MQTT_DIR "/home/jtao//thirdparty/mqtt/src")
set(EIGEN_DIR "/home/jtao//thirdparty/eigen3")

# ######################## configures here to different types of executable ##########################
set(USE_PARALLEL_MKL OFF)
set(USE_SEQ_MKL ON)
set(USE_REDIS ON)
set(USE_MQTT  OFF)

# ####################################################################################################
if(USE_PARALLEL_MKL)
    message(STATUS ${USE_PARALLEL_MKL})
    if(USE_SEQ_MKL)
        message(FATAL_ERROR "`use parallel mkl libraries` and `use sequent mkl libraries` can't be set ON at the same time")
    endif(USE_SEQ_MKL)
endif(USE_PARALLEL_MKL)


INCLUDE_DIRECTORIES("${PROJECT_BINARY_DIR}")
INCLUDE_DIRECTORIES("${PROJECT_SOURCE_DIR}")
set(CMAKE_CXX_STANDARD 17)

set(bbo_libs)
# source files 
# FRAME 
FILE(GLOB SOURCE_CONFIG_BAMBOO ${PROJECT_SOURCE_DIR}/src/Frame/Config/Controller/*.cpp)
FILE(GLOB SOURCE_CONFIG_UTILS_COM ${PROJECT_SOURCE_DIR}/src/Frame/Config/Utils/Com/*.cpp)
FILE(GLOB SOURCE_CONFIG_UTILS_JSON ${PROJECT_SOURCE_DIR}/src/Frame/Config/Utils/Json/*.cpp)

FILE(GLOB SOURCE_CONNECTION_INNERLINK ${PROJECT_SOURCE_DIR}/src/Frame/Connection/InnerLink/*.cpp)
FILE(GLOB SOURCE_CONNECTION_NET ${PROJECT_SOURCE_DIR}/src/Frame/Connection/Net/*.cpp)
FILE(GLOB SOURCE_FRAME_CONNECTION_BOOTLOADER ${PROJECT_SOURCE_DIR}/src/Frame/Connection/Bootloader/*.cpp)

FILE(GLOB SOURCE_GNSS_IONO ${PROJECT_SOURCE_DIR}/src/Frame/Gnss/GnssInput/Atomsphere/*.cpp)
FILE(GLOB SOURCE_GNSS_IONO_READER ${PROJECT_SOURCE_DIR}/src/Frame/Gnss/GnssInput/Atomsphere/Reader/*.cpp)
FILE(GLOB SOURCE_GNSS_DCB_READER ${PROJECT_SOURCE_DIR}/src/Frame/Gnss/GnssInput/DCB/*.cpp)
FILE(GLOB SOURCE_GNSS_OBS ${PROJECT_SOURCE_DIR}/src/Frame/Gnss/GnssInput/GnssObs/*.cpp)
FILE(GLOB SOURCE_GNSS_ORBCLK_ADAPTER ${PROJECT_SOURCE_DIR}/src/Frame/Gnss/GnssInput/Orbclk/Adapter/*.cpp)
FILE(GLOB SOURCE_GNSS_ORBCLK_READER ${PROJECT_SOURCE_DIR}/src/Frame/Gnss/GnssInput/Orbclk/Reader/*.cpp)

set(SOURCE_FRAME ${SOURCE_CONFIG_BAMBOO} ${SOURCE_GNSS_DCB_READER} ${SOURCE_CONFIG_UTILS_COM}  ${SOURCE_CONFIG_UTILS_JSON} 
${SOURCE_CONNECTION_INNERLINK} ${SOURCE_CONNECTION_NET} ${SOURCE_GNSS_IONO} ${SOURCE_GNSS_IONO_READER}
${SOURCE_FRAME_CONNECTION_BOOTLOADER} ${SOURCE_GNSS_OBS} ${SOURCE_GNSS_ORBCLK_ADAPTER}
 ${SOURCE_GNSS_ORBCLK_READER})


FILE(GLOB SOURCE_APPLICATIONS_INTERPOLATE ${PROJECT_SOURCE_DIR}/src/Applications/Interpolate/*.cpp)
FILE(GLOB SOURCE_APPLICATIONS_INTERPOLATE_CONNECTION ${PROJECT_SOURCE_DIR}/src/Applications/Interpolate/Connection/*.cpp)
FILE(GLOB SOURCE_APPLICATIONS_SURFACEMODEL ${PROJECT_SOURCE_DIR}/src/Applications/SurfaceModel/*.cpp)
FILE(GLOB SOURCE_APPLICATIONS_ENCODER ${PROJECT_SOURCE_DIR}/src/Applications/Encoder/*.cpp)


set(SOURCE_APPLICATIONS ${SOURCE_APPLICATIONS_ROTI} ${SOURCE_APPLICATIONS_INTERPOLATE} ${SOURCE_APPLICATIONS_INTERPOLATE_CONNECTION} ${SOURCE_APPLICATIONS_SURFACEMODEL} ${SOURCE_APPLICATIONS_ENCODER})

set(SOURCE_ALL ${PROJECT_SOURCE_DIR}/main.cpp ${SOURCE_FRAME} ${SOURCE_APPLICATIONS})
# add MKL library
if(USE_PARALLEL_MKL)
    INCLUDE_DIRECTORIES("${MKLROOT}/include")
    set(bbo_libs ${bbo_libs}
            ${MKLROOT}/lib//libmkl_scalapack_lp64.a
            -Wl,--start-group
            ${MKLROOT}/lib//libmkl_intel_lp64.a
            ${MKLROOT}//lib//libmkl_intel_thread.a
            ${MKLROOT}//lib//libmkl_core.a
            ${MKLROOT}//lib//libmkl_blacs_intelmpi_lp64.a
            -Wl,--end-group
            ${MKLLIB}/libiomp5.a
            -ldl)
endif(USE_PARALLEL_MKL)

if(USE_SEQ_MKL)
    INCLUDE_DIRECTORIES("${MKLROOT}/include")
    set(bbo_libs ${bbo_libs}
            -Wl,--start-group
            ${MKLROOT}//lib//libmkl_intel_lp64.a
            ${MKLROOT}//lib//libmkl_sequential.a
            ${MKLROOT}//lib//libmkl_core.a
            -Wl,--end-group
            -ldl)
endif(USE_SEQ_MKL)

if(USE_MYSQL)
    set(bbo_libs ${bbo_libs} ${MYSQL_DIR}/libmysqlclient.a)
endif()

if(USE_REDIS)
    set(bbo_libs ${bbo_libs} ${REDIS_DIR}/lib/libhiredis.a)
    INCLUDE_DIRECTORIES("${REDIS_DIR}/include/")
endif()

if(USE_MQTT)
set(bbo_libs  ${bbo_libs} ${MQTT_DIR}/libpaho-mqtt3c.a)
INCLUDE_DIRECTORIES("${MQTT_DIR}")
endif()


##mandatory
INCLUDE_DIRECTORIES("${EIGEN_DIR}")
set(bbo_libs  ${bbo_libs} ${GSL_DIR}/lib/libgsl.a ${GSL_DIR}/lib/libgslcblas.a)
INCLUDE_DIRECTORIES("${GSL_DIR}/include/")

set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -g")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -ggdb -lm")
configure_file(${PROJECT_SOURCE_DIR}/bbo_build.config.in ${PROJECT_SOURCE_DIR}/bbo_build.config.h)




add_executable(iono ${SOURCE_ALL})


target_link_libraries(iono ${bbo_libs})
target_link_libraries(iono pthread)

 #set(PAHO_MQTT_C_PATH /usr/loca/lib64/)
 #target_link_libraries(bamboo  /usr/local/lib64//libpaho-mqtt3c.so)

