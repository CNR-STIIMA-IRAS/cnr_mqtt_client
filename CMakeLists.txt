#############################
## PRELIMINARIES           ##
#############################
cmake_minimum_required(VERSION 3.0.2)

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
option(USE_ROS1                 "ROS SUPPORT" ON)
option(ENABLE_TESTING           "ENABLE TESTING" OFF)
option(ENABLE_COVERAGE_TESTING  "ENABLE COVERAGE TESTING" ON)
option(COMPILE_EXAMPLE          "COMPILE THE EXAMPLE" OFF)

if(USE_ROS1)
  set(ENABLE_TESTING ${CATKIN_ENABLE_TESTING})
endif()
#############################
## END - OPTIONS           ##
#############################


#############################
## DEPENDANCIES            ##
#############################
if(USE_ROS1)
  find_package(catkin REQUIRED)

  catkin_package(
    INCLUDE_DIRS include
    LIBRARIES cnr_mqtt_client
    DEPENDS mosquitto
  )
endif()

cnr_install_directories(USE_ROS1 
  CNR_INSTALL_INCLUDE_DIR CNR_INSTALL_LIB_DIR CNR_INSTALL_BIN_DIR CNR_INSTALL_SHARE_DIR)
cnr_enable_testing(ENABLE_TESTING ENABLE_COVERAGE USE_ROS1)
#############################
## END DEPENDANCIES        ##
#############################



#################
## Build       ##
#################
if(USE_ROS1)
  list(APPEND DEPENDENCIES_INCLUDE_DIRS ${catkin_INCLUDE_DIRS})

  if(WIN32)
  list(APPEND DEPENDENCIES_INCLUDE_DIRS "C:\\ws\\mosquitto\\include\\")
  endif()
endif()

list(APPEND BUILD_INTERFACE_INCLUDE_DIRS ${DEPENDENCIES_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/include)
list(APPEND INSTALL_INTERFACE_INCLUDE_DIRS include ${DEPENDENCIES_INCLUDE_DIRS} include)

if(USE_ROS1)
  list(APPEND DEPENDENCIES_LIBRARIES ${catkin_LIBRARIES})

  if(WIN32)
    list(APPEND DEPENDENCIES_LIBRARIES "C:\\ws\\mosquitto-build\\lib\\")
  endif()
endif()

# ================================================
add_library(${PROJECT_NAME} SHARED
              src/cnr_mqtt_client.cpp
                src/dynamic_callback.cpp)
target_include_directories(${PROJECT_NAME}  PUBLIC
  "$<BUILD_INTERFACE:${BUILD_INTERFACE_INCLUDE_DIRS}>"
    "$<INSTALL_INTERFACE:${INSTALL_INTERFACE_INCLUDE_DIRS}>")

target_link_libraries(${PROJECT_NAME} PUBLIC ${DEPENDENCIES_LIBRARIES})

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

install(TARGETS cnr_mqtt_client
        EXPORT export_cnr_mqtt_client
        ARCHIVE DESTINATION ${CNR_INSTALL_LIB_DIR}
        LIBRARY DESTINATION ${CNR_INSTALL_LIB_DIR}
        RUNTIME DESTINATION ${CNR_INSTALL_BIN_DIR}
)

if(USE_ROS1)
  install(DIRECTORY launch/
  DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}/launch
  PATTERN ".svn" EXCLUDE)
endif()
###################
## END - Install ##
###################

##################################
## CMAKE CONFIG (IF NOT CATKIN) ##
##################################
if(NOT USE_ROS1) 
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
  # include(GoogleTest)
  # add_executable(cnr_mqtt_client_server_test 
  #   src/cnr_mqtt_client_server/utils/args_parser.cpp
  #   src/cnr_mqtt_client_server/utils/yaml_manager.cpp
  #   test/test_server.cpp)
  # if(${CMAKE_VERSION} VERSION_GREATER  "3.16.0")
  #   target_link_libraries(cnr_mqtt_client_server_test cnr_mqtt_client
  #      Threads::Threads GTest::gtest_main Boost::program_options Boost::system Boost::filesystem Boost::iostreams Boost::regex)
  # else()
  #   target_link_libraries(cnr_mqtt_client_server_test cnr_mqtt_client GTest::gtest_main)
  #   if(THREADS_HAVE_PTHREAD_ARG)
  #     target_compile_options(cnr_mqtt_client_server_test PUBLIC "-pthread")
  #   endif()
  #   if(CMAKE_THREAD_LIBS_INIT)
  #     target_link_libraries(cnr_mqtt_client_server_test "${CMAKE_THREAD_LIBS_INIT}")
  #   endif()
  # endif()
  # target_compile_definitions(cnr_mqtt_client_server_test PRIVATE TEST_DIR="${CMAKE_CURRENT_LIST_DIR}/test")  
  # gtest_discover_tests(cnr_mqtt_client_server_test)
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

