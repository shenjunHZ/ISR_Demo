#Copyright (c) 2019 junshen

set(SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/Main.cpp
        ${CMAKE_CURRENT_LIST_DIR}/configurations/ConfigurationLoader.cpp
        ${CMAKE_CURRENT_LIST_DIR}/applications/AppInstance.cpp
        ${CMAKE_CURRENT_LIST_DIR}/applications/ISRService.cpp
        ${CMAKE_CURRENT_LIST_DIR}/applications/WinRec.cpp
        ${CMAKE_CURRENT_LIST_DIR}/applications/AppConfig.cpp
        ${CMAKE_CURRENT_LIST_DIR}/applications/commonControl.cpp
        ${CMAKE_CURRENT_LIST_DIR}/qtApplications/MainUI.cpp
)

set(HEADERS
        ${CMAKE_CURRENT_LIST_DIR}/configurations/ConfigurationLoader.hpp
        ${CMAKE_CURRENT_LIST_DIR}/configurations/AppConfigurations.hpp
        ${CMAKE_CURRENT_LIST_DIR}/applications/AppInstance.hpp
        ${CMAKE_CURRENT_LIST_DIR}/applications/IISRService.hpp
        ${CMAKE_CURRENT_LIST_DIR}/applications/ISRService.hpp
        ${CMAKE_CURRENT_LIST_DIR}/applications/ISysRec.hpp
        ${CMAKE_CURRENT_LIST_DIR}/applications/WinRec.hpp
        ${CMAKE_CURRENT_LIST_DIR}/applications/AppConfig.hpp
        ${CMAKE_CURRENT_LIST_DIR}/applications/commonControl.hpp
        ${CMAKE_CURRENT_LIST_DIR}/qtApplications/MainUI.hpp
)

file(GLOB MOCS_HEAD "${CMAKE_CURRENT_LIST_DIR}/qtApplications/*.hpp")
file(GLOB MOCS_SRC "${CMAKE_CURRENT_LIST_DIR}/qtApplications/*.cpp")
QT5_WRAP_CPP(MOCS ${MOCS_SRC} ${MOCS_HEAD})

file(GLOB FORMS "${CMAKE_CURRENT_LIST_DIR}/qtApplications/*.ui")
QT5_WRAP_UI(FORMS_HEADERS ${FORMS})

add_executable(${EXECUTABLE_NAME} ${SOURCES} ${HEADERS} ${FORMS_HEADERS} ${MOCS})
#set output files path
SET_TARGET_PROPERTIES(${EXECUTABLE_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../bin)

 # Use the Widgets module from Qt 5, not need after use find_package
 #qt5_use_modules(${EXECUTABLE_NAME} Widgets Core Gui)

target_link_libraries(${EXECUTABLE_NAME}
    PRIVATE
        ${LIBISR}
        ${WINLIB}
        #${BOOST_PROGRAM_OPTIONS}
        #${BOOST_SYSTEM}
        ${LIBCOMMON}
        ${LIBDSGUI}
        ${Qt5Core_LIBRARIES}
        ${Qt5Gui_LIBRARIES}
        ${Qt5Widgets_LIBRARIES}
        logger
        timer
)

ADD_SUBDIRECTORY(logger)
ADD_SUBDIRECTORY(timer)

# TODO: 如有需要，请添加测试并安装目标。
