# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
# 并包含子项目。
#
#Copyright (c) 2019 junshen

cmake_minimum_required (VERSION 3.8)

project ("ISR_Demo")
set(EXECUTABLE_NAME "ISR_Demo")

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(WINDOWS TRUE)
    MESSAGE(STATUS "Windows env.")
endif()

# Find includes in corresponding build directories
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)

if(${WINDOWS})
    INCLUDE_DIRECTORIES(
        "D:\\Develop_Code\\Open_Resource\\boost_1_65_1"
        "D:\\Develop_Code\\Open_Resource\\spdlog-1.x\\include"
        "D:\\Develop_Code\\Open_Resource\\qt_5.10.1_32\\5.10.1\\msvc2015\\include"
        #"C:\\Qt\\Qt5.7.1_32\\5.7\\msvc2015\\include"
        "D:\\Develop_Code\\Open_Resource\\DCMGui"
        "D:\\Develop_Code\\Open_Resource\\MyInterface"
        ${CMAKE_CURRENT_LIST_DIR}/Windows_iat1226_5d3807fe/include
    )
    LINK_DIRECTORIES(
        "D:\\Develop_Code\\Open_Resource\\boost_1_65_1\\lib32-msvc-14.1"
        "D:\\Develop_Code\\Open_Resource\\qt_5.10.1_32\\5.10.1\\msvc2015\\lib"
        "D:\\Develop_Code\\Open_Resource\\MyInterface"
        ${CMAKE_CURRENT_LIST_DIR}/Windows_iat1226_5d3807fe/libs
    )

    set(LIBISR msc)
    set(LIBCOMMON Common)
    set(LIBDSGUI DSGui)
    #set(Boost_USE_STATIC_LIBS TRUE)
    #set(Boost_USE_STATIC_RUNTIME TRUE)
    #find_package(PkgConfig)
    set(BOOST_ROOT D:\\Develop_Code\\Open_Resource\\boost_1_65_1)
    find_package(Boost 1.65.0 REQUIRED COMPONENTS system filesystem)
    #set(BOOST_PROGRAM_OPTIONS boost_program_options-vc141-mt-1_65_1)
    #set(BOOST_SYSTEM boost_system-vc141-mt-1_65_1)

    # 添加寻找.cmake文件的路径，有时候不需要写下面这几行
    #set(QT_CMAKE_DIR "D:\\Develop_Code\\Open_Resource\\qt_5.10.1_32\\5.10.1\\msvc2015\\lib\\cmake")
    set(QT_PREFIX_DIR "D:\\Develop_Code\\Open_Resource\\qt_5.10.1_32\\5.10.1\\msvc2015")
    # 寻找Qt5Config.cmake的路径
    ##list(APPEND CMAKE_MODULE_PATH ${QT_CMAKE_DIR}/Qt5)
    # 寻找Qt5WidgetsConfig.cmake的路径
    #list(APPEND CMAKE_MODULE_PATH ${QT_CMAKE_DIR}/Qt5Widgets)
    #list(APPEND CMAKE_MODULE_PATH ${QT_CMAKE_DIR}/Qt5Core)
    #list(APPEND CMAKE_MODULE_PATH ${QT_CMAKE_DIR}/Qt5Gui)
 
 # need to be include in separate modules
    #MESSAGE(STATUS ${CMAKE_PREFIX_PATH})
    #set(PREFIX_PATH ${CMAKE_PREFIX_PATH})
    set(CMAKE_PREFIX_PATH ${QT_PREFIX_DIR})

    find_package(Qt5Core REQUIRED)
    find_package(Qt5Gui REQUIRED)
    find_package(Qt5Widgets REQUIRED)
    SET(RCC_EXECUTABLE ${CMAKE_PREFIX_PATH}/bin/rcc.exe)

 #   include_directories(${Qt5Core_INCLUDE_DIRS} ${Qt5Gui_INCLUDE_DIRS} ${Qt5Widgets_INCLUDE_DIRS})

    set(WINLIB winmm)
endif()

#message(CURRENT_BINARY_DIR = ${CMAKE_CURRENT_BINARY_DIR})
INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

if(NOT ${WINDOWS})
    add_compile_options(
        -g
    )
    SET(COMMON_FLAGS ${CMAKE_CXX_FLAGS})
    set(CMAKE_CXX_FLAGS "${COMMON_FLAGS} -std=c++14")
endif()

# 包含子项目。
add_subdirectory ("src")