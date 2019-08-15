set(MODULE_NAME timer)
project(${MODULE_NAME} CXX)

message(STATUS "Configuring ${MODULE_NAME}")

set(SOURCES
        src/DefaultTimerService.cpp
        src/IOService.cpp)

set(HEADERS
        include/DefaultTimerService.hpp
        include/IOService.hpp
        include/Timer.hpp
        include/TimerIdGenerator.hpp
        include/TimerService.hpp)

add_library(${MODULE_NAME} STATIC ${SOURCES} ${HEADERS})

target_include_directories(${MODULE_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        )

target_link_libraries(${MODULE_NAME}
        PUBLIC
            logger

        #PRIVATE
        #${Boost_LIBRARIES}
        )