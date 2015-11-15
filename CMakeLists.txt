cmake_minimum_required(VERSION 2.8.3)
project(pick_n_place)

## Find catkin macros and libraries
## if COMPONENTS list like find_package(catkin REQUIRED COMPONENTS xyz)
## is used, also find other catkin packages
find_package(catkin REQUIRED COMPONENTS
  roscpp
  rospy
  std_msgs
)

#####################################################################################
#find object
####################################################################################

#cmake_minimum_required(VERSION 2.8.3)
#project(find_object_2d)

## Find catkin macros and libraries
## if COMPONENTS list like find_package(catkin REQUIRED COMPONENTS xyz)
## is used, also find other catkin packages
find_package(catkin REQUIRED COMPONENTS 
             cv_bridge roscpp rospy sensor_msgs std_msgs image_transport genmsg message_filters pcl_ros tf
)

## Generate messages in the 'msg' folder
add_message_files(
   FILES
   ObjectsStamped.msg
)

## Generate added messages and services with any dependencies listed here
generate_messages(
   DEPENDENCIES
   std_msgs
   sensor_msgs
)

###################################
## catkin specific configuration ##
###################################
## The catkin_package macro generates cmake config files for your package
## Declare things to be passed to dependent projects
## INCLUDE_DIRS: uncomment this if you package contains header files
## LIBRARIES: libraries you create in this project that dependent projects also need
## CATKIN_DEPENDS: catkin_packages dependent projects also need
## DEPENDS: system dependencies of this project that dependent projects also need
catkin_package(
  CATKIN_DEPENDS cv_bridge roscpp rospy sensor_msgs std_msgs image_transport message_filters pcl_ros tf
)

###########
## Build ##
###########
SET(PROJECT_VERSION “0.5.1”)
SET(PROJECT_PREFIX find_object)

STRING(REGEX MATCHALL "[0-9]" PROJECT_VERSION_PARTS "${PROJECT_VERSION}")
LIST(GET PROJECT_VERSION_PARTS 0 PROJECT_VERSION_MAJOR)
LIST(GET PROJECT_VERSION_PARTS 1 PROJECT_VERSION_MINOR)
LIST(GET PROJECT_VERSION_PARTS 2 PROJECT_VERSION_PATCH)

ADD_DEFINITIONS(-DPROJECT_PREFIX="${PROJECT_PREFIX}")
ADD_DEFINITIONS(-DPROJECT_VERSION="${PROJECT_VERSION}")

find_package(Qt4 COMPONENTS QtCore QtGui QtNetwork)
FIND_PACKAGE(OpenCV REQUIRED) # tested on 2.3.1

IF(OPENCV_NONFREE_FOUND)
  SET(NONFREE 1)
ELSE()
  SET(NONFREE 0)
ENDIF()
CONFIGURE_FILE(Version.h.in ${PROJECT_SOURCE_DIR}/include_x/${PROJECT_PREFIX}/Version.h)

### Qt Gui stuff ###
SET(headers_ui 
	./include_x/find_object/MainWindow.h
	./include_x/find_object/TcpServer.h
	./include_x/find_object/ObjWidget.h
	./include_x/find_object/Camera.h
	./include_x/find_object/FindObject.h
	./src_x/AddObjectDialog.h
	./src_x/ParametersToolBox.h
	./src_x/RectItem.h
	./src_x/CameraTcpServer.h
	./src/CameraROS.h
	./src/FindObjectROS.h
	./src_x/AboutDialog.h
	./src_x/utilite/UPlot.h
	./src_x/rtabmap/PdfPlot.h
	./src_x/ImageDropWidget.h
)

SET(uis
    ./src_x/ui/mainWindow.ui
    ./src_x/ui/addObjectDialog.ui
    ./src_x/ui/aboutDialog.ui
)

SET(qrc 
    ./src_x/resources.qrc
)

# generate rules for building source files from the resources
QT4_ADD_RESOURCES(srcs_qrc ${qrc})

#Generate .h files from the .ui files
QT4_WRAP_UI(moc_uis ${uis})

#This will generate moc_* for Qt
QT4_WRAP_CPP(moc_srcs ${headers_ui})
### Qt Gui stuff  end###


SET(SRC_FILES 
	./src_x/MainWindow.cpp
	./src_x/AddObjectDialog.cpp
	./src_x/KeypointItem.cpp
	./src_x/QtOpenCV.cpp
	./src_x/Camera.cpp
	./src_x/CameraTcpServer.cpp
	./src/CameraROS.cpp
	./src/FindObjectROS.cpp
	./src_x/ParametersToolBox.cpp
	./src_x/Settings.cpp
	./src_x/FindObject.cpp
	./src_x/ObjWidget.cpp
	./src_x/TcpServer.cpp
	./src_x/Vocabulary.cpp
	./src_x/RectItem.cpp
	./src_x/AboutDialog.cpp
	./src_x/JsonWriter.cpp
	./src_x/utilite/UPlot.cpp
	./src_x/utilite/UFile.cpp
	./src_x/utilite/UDirectory.cpp
	./src_x/utilite/UConversion.cpp
	./src_x/utilite/ULogger.cpp
	./src_x/rtabmap/PdfPlot.cpp
	./src_x/ImageDropWidget.cpp
	./src_x/json/jsoncpp.cpp
	${moc_srcs} 
	${moc_uis} 
	${srcs_qrc}
)

SET(INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/src
	${CMAKE_CURRENT_SOURCE_DIR}/src_x
	${CMAKE_CURRENT_SOURCE_DIR}/include_x
	${CMAKE_CURRENT_BINARY_DIR} # for qt ui generated in binary dir
	${catkin_INCLUDE_DIRS}
)

INCLUDE(${QT_USE_FILE})
ADD_DEFINITIONS(-DQT_NO_KEYWORDS)

SET(LIBRARIES
    ${QT_LIBRARIES} 
    ${catkin_LIBRARIES}
)

#include files
INCLUDE_DIRECTORIES(${INCLUDE_DIRS})

add_executable(find_object_2d src/find_object_2d_node.cpp ${SRC_FILES})
target_link_libraries(find_object_2d ${LIBRARIES})
add_dependencies(find_object_2d find_object_2d_generate_messages_cpp)

add_executable(print_objects_detected src/print_objects_detected_node.cpp)
target_link_libraries(print_objects_detected ${LIBRARIES})
add_dependencies(print_objects_detected find_object_2d_generate_messages_cpp)

add_executable(tf_example src/tf_example_node.cpp)
target_link_libraries(tf_example ${LIBRARIES})
add_dependencies(tf_example find_object_2d_generate_messages_cpp)

#############
## Install ##
#############
## Mark executables and/or libraries for installation
install(TARGETS 
   find_object_2d 
   print_objects_detected 
   tf_example 
   ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
   LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
   RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
 )

## Mark other files for installation (e.g. launch and bag files, etc.)
install(FILES
   launch/find_object_2d_gui.launch
   launch/find_object_2d.launch
   launch/find_object_3d.launch
   DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}/launch
)
## System dependencies are found with CMake's conventions
# find_package(Boost REQUIRED COMPONENTS system)


## Uncomment this if the package has a setup.py. This macro ensures
## modules and global scripts declared therein get installed
## See http://ros.org/doc/api/catkin/html/user_guide/setup_dot_py.html
# catkin_python_setup()

################################################
## Declare ROS messages, services and actions ##
################################################

## To declare and build messages, services or actions from within this
## package, follow these steps:
## * Let MSG_DEP_SET be the set of packages whose message types you use in
##   your messages/services/actions (e.g. std_msgs, actionlib_msgs, ...).
## * In the file package.xml:
##   * add a build_depend tag for "message_generation"
##   * add a build_depend and a run_depend tag for each package in MSG_DEP_SET
##   * If MSG_DEP_SET isn't empty the following dependency has been pulled in
##     but can be declared for certainty nonetheless:
##     * add a run_depend tag for "message_runtime"
## * In this file (CMakeLists.txt):
##   * add "message_generation" and every package in MSG_DEP_SET to
##     find_package(catkin REQUIRED COMPONENTS ...)
##   * add "message_runtime" and every package in MSG_DEP_SET to
##     catkin_package(CATKIN_DEPENDS ...)
##   * uncomment the add_*_files sections below as needed
##     and list every .msg/.srv/.action file to be processed
##   * uncomment the generate_messages entry below
##   * add every package in MSG_DEP_SET to generate_messages(DEPENDENCIES ...)

## Generate messages in the 'msg' folder
# add_message_files(
#   FILES
#   Message1.msg
#   Message2.msg
# )

## Generate services in the 'srv' folder
# add_service_files(
#   FILES
#   Service1.srv
#   Service2.srv
# )

## Generate actions in the 'action' folder
# add_action_files(
#   FILES
#   Action1.action
#   Action2.action
# )

## Generate added messages and services with any dependencies listed here
# generate_messages(
#   DEPENDENCIES
#   std_msgs
# )

################################################
## Declare ROS dynamic reconfigure parameters ##
################################################

## To declare and build dynamic reconfigure parameters within this
## package, follow these steps:
## * In the file package.xml:
##   * add a build_depend and a run_depend tag for "dynamic_reconfigure"
## * In this file (CMakeLists.txt):
##   * add "dynamic_reconfigure" to
##     find_package(catkin REQUIRED COMPONENTS ...)
##   * uncomment the "generate_dynamic_reconfigure_options" section below
##     and list every .cfg file to be processed

## Generate dynamic reconfigure parameters in the 'cfg' folder
# generate_dynamic_reconfigure_options(
#   cfg/DynReconf1.cfg
#   cfg/DynReconf2.cfg
# )

###################################
## catkin specific configuration ##
###################################
## The catkin_package macro generates cmake config files for your package
## Declare things to be passed to dependent projects
## INCLUDE_DIRS: uncomment this if you package contains header files
## LIBRARIES: libraries you create in this project that dependent projects also need
## CATKIN_DEPENDS: catkin_packages dependent projects also need
## DEPENDS: system dependencies of this project that dependent projects also need
catkin_package(
#  INCLUDE_DIRS include
#  LIBRARIES pick_n_place
#  CATKIN_DEPENDS roscpp rospy std_msgs
#  DEPENDS system_lib
)

###########
## Build ##
###########

## Specify additional locations of header files
## Your package locations should be listed before other locations
# include_directories(include)
include_directories(
  ${catkin_INCLUDE_DIRS}
)

## Declare a C++ library
# add_library(pick_n_place
#   src/${PROJECT_NAME}/pick_n_place.cpp
# )

## Add cmake target dependencies of the library
## as an example, code may need to be generated before libraries
## either from message generation or dynamic reconfigure
# add_dependencies(pick_n_place ${${PROJECT_NAME}_EXPORTED_TARGETS} ${catkin_EXPORTED_TARGETS})

## Declare a C++ executable
# add_executable(pick_n_place_node src/pick_n_place_node.cpp)

## Add cmake target dependencies of the executable
## same as for the library above
# add_dependencies(pick_n_place_node ${${PROJECT_NAME}_EXPORTED_TARGETS} ${catkin_EXPORTED_TARGETS})

## Specify libraries to link a library or executable target against
# target_link_libraries(pick_n_place_node
#   ${catkin_LIBRARIES}
# )

#############
## Install ##
#############

# all install targets should use catkin DESTINATION variables
# See http://ros.org/doc/api/catkin/html/adv_user_guide/variables.html

## Mark executable scripts (Python etc.) for installation
## in contrast to setup.py, you can choose the destination
# install(PROGRAMS
#   scripts/my_python_script
#   DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
# )

## Mark executables and/or libraries for installation
# install(TARGETS pick_n_place pick_n_place_node
#   ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
#   LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
#   RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
# )

## Mark cpp header files for installation
# install(DIRECTORY include/${PROJECT_NAME}/
#   DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION}
#   FILES_MATCHING PATTERN "*.h"
#   PATTERN ".svn" EXCLUDE
# )

## Mark other files for installation (e.g. launch and bag files, etc.)
# install(FILES
#   # myfile1
#   # myfile2
#   DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}
# )

#############
## Testing ##
#############

## Add gtest based cpp test target and link libraries
# catkin_add_gtest(${PROJECT_NAME}-test test/test_pick_n_place.cpp)
# if(TARGET ${PROJECT_NAME}-test)
#   target_link_libraries(${PROJECT_NAME}-test ${PROJECT_NAME})
# endif()

## Add folders to be run by python nosetests
# catkin_add_nosetests(test)
