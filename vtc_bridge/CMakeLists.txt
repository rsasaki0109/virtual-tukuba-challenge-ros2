cmake_minimum_required(VERSION 3.8)
project(vtc_bridge)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(rclcpp REQUIRED)
find_package(vtc_msgs REQUIRED)

set(VTC_CAGECLIENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/CageClient" CACHE PATH
  "Path to a CageClient checkout")
set(VTC_LOCAL_ZMQ_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/libzmq3-dev/usr" CACHE PATH
  "Path to an extracted local libzmq3-dev prefix")

add_executable(vtc_bridge_node
  src/mock_simulator_transport.cpp
  src/transport_factory.cpp
  src/vtc_bridge_node.cpp
)
target_include_directories(vtc_bridge_node PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}>)
target_compile_features(vtc_bridge_node PUBLIC c_std_99 cxx_std_17)  # Require C99 and C++17
ament_target_dependencies(
  vtc_bridge_node
  "geometry_msgs"
  "nav_msgs"
  "sensor_msgs"
  "tf2"
  "tf2_ros"
  "rclcpp"
  "vtc_msgs"
)

find_path(VTC_ZMQ_INCLUDE_DIR zmq.h
  HINTS
    "${VTC_LOCAL_ZMQ_PREFIX}/include"
)
set(VTC_ZMQ_LIBRARY "")
if(EXISTS "${VTC_LOCAL_ZMQ_PREFIX}/lib/x86_64-linux-gnu/libzmq.so")
  set(VTC_ZMQ_LIBRARY "${VTC_LOCAL_ZMQ_PREFIX}/lib/x86_64-linux-gnu/libzmq.so")
elseif(EXISTS "/lib/x86_64-linux-gnu/libzmq.so.5")
  set(VTC_ZMQ_LIBRARY "/lib/x86_64-linux-gnu/libzmq.so.5")
else()
  find_library(VTC_ZMQ_LIBRARY NAMES zmq libzmq
    HINTS
      "${VTC_LOCAL_ZMQ_PREFIX}/lib"
      "${VTC_LOCAL_ZMQ_PREFIX}/lib/x86_64-linux-gnu"
  )
endif()

set(VTC_CAGECLIENT_HEADER "${VTC_CAGECLIENT_DIR}/include/cageclient.hh")
set(VTC_CAGECLIENT_JSON_INCLUDE "${VTC_CAGECLIENT_DIR}/json/include")
find_path(VTC_NLOHMANN_JSON_INCLUDE_DIR nlohmann/json.hpp)

if(EXISTS "${VTC_CAGECLIENT_HEADER}" AND VTC_ZMQ_INCLUDE_DIR AND VTC_ZMQ_LIBRARY)
  message(STATUS "Enabling CageClient backend from ${VTC_CAGECLIENT_DIR}")
  target_sources(vtc_bridge_node PRIVATE src/cageclient_simulator_transport.cpp)
  target_compile_definitions(vtc_bridge_node PRIVATE VTC_BRIDGE_HAS_CAGECLIENT=1)
  target_include_directories(vtc_bridge_node PRIVATE
    "${VTC_CAGECLIENT_DIR}/include"
    "${VTC_ZMQ_INCLUDE_DIR}"
  )
  if(EXISTS "${VTC_CAGECLIENT_JSON_INCLUDE}")
    target_include_directories(vtc_bridge_node PRIVATE "${VTC_CAGECLIENT_JSON_INCLUDE}")
  elseif(VTC_NLOHMANN_JSON_INCLUDE_DIR)
    target_include_directories(vtc_bridge_node PRIVATE "${VTC_NLOHMANN_JSON_INCLUDE_DIR}")
  endif()
  target_link_libraries(vtc_bridge_node "${VTC_ZMQ_LIBRARY}")
else()
  message(STATUS
    "CageClient backend disabled. Provide CageClient at ${VTC_CAGECLIENT_DIR} and install libzmq3-dev "
    "to enable transport_backend:=cageclient.")
endif()

install(DIRECTORY include/
  DESTINATION include/${PROJECT_NAME}
)

install(TARGETS vtc_bridge_node
  DESTINATION lib/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
