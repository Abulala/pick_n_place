cmake_minimum_required(VERSION 2.8.3)
project(torque_example)

find_package(catkin REQUIRED COMPONENTS
  brics_actuator
  tf_conversions
  roscpp
  ik_solver_service
  brics_actuator
  trajectory_msgs
  rpg_youbot_common
  torque_control
  trajectory_generator
  message_generation
  cmake_modules

  tf
)

find_package(Eigen REQUIRED)
find_package(Boost REQUIRED)
find_package(Qt4 COMPONENTS QtCore)

catkin_package(
  CATKIN_DEPENDS
    torque_control
    brics_actuator
    trajectory_msgs
    message_runtime
)

include(${QT_USE_FILE})

include_directories(
  ${catkin_INCLUDE_DIRS}
  ${Eigen_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
  ${QT_INCLUDES}  
)


add_executable(gwd_traj src/ltr.cpp)
add_dependencies(gwd_traj ${catkin_EXPORTED_TARGETS})

target_link_libraries(gwd_traj
  ${catkin_LIBRARIES}
  ${Eigen_LIBRARIES}
  ${Boost_LIBRARIES}
)

add_executable(project2015 src/project15.cpp)
add_dependencies(project2015 ${catkin_EXPORTED_TARGETS})

target_link_libraries(project2015
  ${catkin_LIBRARIES}
  ${Eigen_LIBRARIES}
  ${Boost_LIBRARIES}
  ${QT_LIBRARIES}
)

add_executable(circle_traj src/circle.cpp)
add_dependencies(circle_traj ${catkin_EXPORTED_TARGETS})

target_link_libraries(circle_traj
  ${catkin_LIBRARIES}
  ${Eigen_LIBRARIES}
  ${Boost_LIBRARIES}
)

