cmake_minimum_required(VERSION 3.0.2)
project(cnr_mqtt_client)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_BUILD_TYPE Release)

if(WIN32)
	add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endif()

find_package(catkin REQUIRED)


catkin_package(
  INCLUDE_DIRS include
  LIBRARIES cnr_mqtt_client
#  CATKIN_DEPENDS
  DEPENDS mosquitto
)

###########
## Build ##
###########

include_directories(
 include
 ${catkin_INCLUDE_DIRS}
)

if(WIN32)
  include_directories("C:\\ws\\mosquitto\\include\\")
endif()

if(NOT DEFINED BUILD_SHARED_LIBS)
  set(BUILD_SHARED_LIBS ON)
endif()

## Declare a C++ library
 add_library(${PROJECT_NAME}
   src/cnr_mqtt_client.cpp
   src/dynamic_callback.cpp
 )

add_dependencies(${PROJECT_NAME} ${${PROJECT_NAME}_EXPORTED_TARGETS} ${catkin_EXPORTED_TARGETS})

## Specify libraries to link a library or executable target against
if(WIN32)
  target_link_directories(${PROJECT_NAME} PUBLIC "C:\\ws\\mosquitto-build\\lib\\")
endif()

target_link_libraries(${PROJECT_NAME}
   ${catkin_LIBRARIES}
   mosquitto
)

#############
## Install ##
#############
install(TARGETS ${PROJECT_NAME}
  ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  RUNTIME DESTINATION ${CATKIN_GLOBAL_BIN_DESTINATION}
)

## Mark cpp header files for installation
install(DIRECTORY include/${PROJECT_NAME}/
  DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION}
  FILES_MATCHING PATTERN "*.h"
  PATTERN ".svn" EXCLUDE
)

## Mark other files for installation (e.g. launch and bag files, etc.)
install(FILES
  # myfile1
  # myfile2
  DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}
)
