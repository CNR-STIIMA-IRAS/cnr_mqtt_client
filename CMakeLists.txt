#############################
## PRELIMINARIES           ##
#############################
cmake_minimum_required(VERSION 3.16.0)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/cnrConfigMacros.cmake)

get_project_name("package.xml" extracted_name extracted_version)

if("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" LESS 3.0.2)
  project(${extracted_name})
else()
  project(${extracted_name} VERSION ${extracted_version} LANGUAGES CXX)
endif()

cnr_set_flags()
#############################
## END - PRELIMINARIES     ##
#############################

#############################
## OPTIONS                 ##
#############################
option(ENABLE_TESTING           "ENABLE TESTING" OFF)
option(ENABLE_COVERAGE_TESTING  "ENABLE COVERAGE TESTING" ON)
option(BUILD_EXAMPLE            "COMPILE THE EXAMPLE" OFF)

set(ROS_VERSION $ENV{ROS_VERSION})
if(ROS_VERSION STREQUAL "")
  message(FATAL_ERROR "No supported version of ROS found!")
  set(USE_ROS OFF)
endif()

if(NOT((ROS_VERSION EQUAL 1) OR (ROS_VERSION EQUAL 2)))
  message(FATAL_ERROR "No supported version of ROS found!")
  set(USE_ROS OFF)
endif()

if((ROS_VERSION EQUAL 1) OR (ROS_VERSION EQUAL 2))
  set(USE_ROS ON)
endif()


if(ROS_VERSION EQUAL 1)
  set(ENABLE_TESTING ${CATKIN_ENABLE_TESTING})

elseif(ROS_VERSION EQUAL 2)
  # to be done if necessary

endif()


#############################
## END - OPTIONS           ##
#############################


#############################
## DEPENDANCIES            ##
#############################

if (NOT WIN32)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(Mosquitto IMPORTED_TARGET libmosquitto REQUIRED)
else()
  list(APPEND DEPENDENCIES_DIRECTORIES "C:\\ws\\mosquitto-build\\lib\\")
  list(APPEND DEPENDENCIES_INCLUDE_DIRS "C:\\ws\\mosquitto\\include\\")
  list(APPEND DEPENDENCIES_LIBRARIES "mosquitto")
endif()


if(ROS_VERSION EQUAL 1)
  find_package(catkin REQUIRED)
  catkin_package(
    INCLUDE_DIRS include
    LIBRARIES cnr_mqtt_client
  )

elseif(ROS_VERSION EQUAL 2)
  find_package(ament_cmake REQUIRED)

endif()

cnr_install_directories(ROS_VERSION CNR_INSTALL_INCLUDE_DIR CNR_INSTALL_LIB_DIR CNR_INSTALL_BIN_DIR CNR_INSTALL_SHARE_DIR)
cnr_enable_testing(ENABLE_TESTING ENABLE_COVERAGE ROS_VERSION)


#############################
## END DEPENDANCIES        ##
#############################



#################
## Build       ##
#################
if(ROS_VERSION EQUAL 1)
  list(APPEND DEPENDENCIES_INCLUDE_DIRS ${catkin_INCLUDE_DIRS})

elseif(ROS_VERSION EQUAL 2)
  # to be done if necessary

  endif()

list(APPEND BUILD_INTERFACE_INCLUDE_DIRS ${DEPENDENCIES_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/include)
list(APPEND INSTALL_INTERFACE_INCLUDE_DIRS include ${DEPENDENCIES_INCLUDE_DIRS})

if(ROS_VERSION EQUAL 1)
  list(APPEND DEPENDENCIES_LIBRARIES ${catkin_LIBRARIES} )

elseif(ROS_VERSION EQUAL 2)
  # to be done if necessary

endif()

# ================================================
include_directories(${INSTALL_INTERFACE_INCLUDE_DIRS})
add_library(${PROJECT_NAME} SHARED  
              src/cnr_mqtt_client.cpp
                src/dynamic_callback.cpp)

if(NOT WIN32)
  target_link_libraries(${PROJECT_NAME} PUBLIC PkgConfig::Mosquitto)
else()
  target_link_directories(${PROJECT_NAME} PUBLIC ${DEPENDENCIES_DIRECTORIES})
  target_link_libraries(${PROJECT_NAME} PUBLIC ${DEPENDENCIES_LIBRARIES})
endif()

if(BUILD_EXAMPLE)
  add_executable(cnr_mqtt_client_example example/example.cpp)
  target_link_libraries(cnr_mqtt_client_example PUBLIC ${PROJECT_NAME})
endif()

#################
## END - Build ##
#################


#############
## Install ##
#############
install(DIRECTORY include/${PROJECT_NAME}
        DESTINATION ${CNR_INSTALL_INCLUDE_DIR}
        FILES_MATCHING PATTERN "*.h"
        PATTERN ".git" EXCLUDE
)

install(TARGETS ${PROJECT_NAME} 
        EXPORT export_${PROJECT_NAME}
        ARCHIVE DESTINATION ${CNR_INSTALL_LIB_DIR}
        LIBRARY DESTINATION ${CNR_INSTALL_LIB_DIR}
        RUNTIME DESTINATION ${CNR_INSTALL_BIN_DIR}
)

if(BUILD_EXAMPLE)
install(TARGETS ${PROJECT_NAME}
        EXPORT export_${PROJECT_NAME}
        ARCHIVE DESTINATION ${CNR_INSTALL_LIB_DIR}
        LIBRARY DESTINATION ${CNR_INSTALL_LIB_DIR}
        RUNTIME DESTINATION ${CNR_INSTALL_BIN_DIR}
)
endif()

if(ROS_VERSION EQUAL 1)
  install(DIRECTORY launch/
  DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}/launch
  PATTERN ".svn" EXCLUDE)

elseif(ROS_VERSION EQUAL 2)
   # to be done if necessary

endif()
###################
## END - Install ##
###################

##################################
## CMAKE CONFIG (IF NOT CATKIN) ##
##################################
if(NOT USE_ROS) 
  set(export_targets ${export_targets};cnr_mqtt_client)
  export(EXPORT export_cnr_mqtt_client
    FILE "${PROJECT_BINARY_DIR}/export_cnr_mqtt_client.cmake")

  # Create the cnr_mqtt_clientConfig.cmake
  set(CONF_INCLUDE_DIRS "${CMAKE_INSTALL_PREFIX}/include")
  configure_file(cnr_mqtt_clientConfig.cmake.in
    "${PROJECT_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/cnr_mqtt_clientConfig.cmake" @ONLY)

  # Create the cnr_mqtt_clientConfigVersion.cmake
  configure_file(cnr_mqtt_clientConfigVersion.cmake.in
    "${PROJECT_BINARY_DIR}/cnr_mqtt_clientConfigVersion.cmake" @ONLY)
  
  install(FILES
    "${PROJECT_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/cnr_mqtt_clientConfig.cmake"
    "${PROJECT_BINARY_DIR}/cnr_mqtt_clientConfigVersion.cmake"
    DESTINATION "share/${PROJECT_NAME}/cmake" COMPONENT dev
  )

  install(EXPORT export_cnr_mqtt_client
    DESTINATION "share/${PROJECT_NAME}/cmake"
    FILE export_cnr_mqtt_client.cmake
    COMPONENT dev
  )
endif()


##########################
## Testing              ##
##########################
if(ENABLE_TESTING)
  include(GoogleTest)
  add_executable(cnr_mqtt_client_test test/test.cpp)
  target_link_libraries(cnr_mqtt_client_test cnr_mqtt_client Threads::Threads GTest::gtest_main)
  target_compile_definitions(cnr_mqtt_client_test PRIVATE TEST_DIR="${CMAKE_CURRENT_LIST_DIR}/test")  
  gtest_discover_tests(cnr_mqtt_client_test)
 endif(ENABLE_TESTING)


##########################
## Coverage Testing     ##
##########################
if( ENABLE_TESTING AND ENABLE_COVERAGE_TESTING AND NOT WIN32 )
  # if(USE_ROS)
  #   message(STATUS "The coverage testing is enabled")
  #   set(COVERAGE_EXCLUDES "*/${PROJECT_NAME}/test*")
  #   add_code_coverage(
  #     NAME coverage_report
  #     DEPENDENCIES cnr_mqtt_client_server_test
  #   )
  # else()
  #   ## NOTHING TO DO, SINCE GTEST ALREADY CREATES THE COVERAGE
  #   ## IF THE COMPILING OPTIONS ARE SET
  # endif()
endif()

