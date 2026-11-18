cmake_minimum_required(VERSION 3.8)
project(cpp_service)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(base_interfaces_demo REQUIRED)
find_package(OpenCV REQUIRED)

# ---------- 服务端 ----------
add_executable(cpp_server src/cpp_server.cpp)

target_include_directories(cpp_server PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(cpp_server ${OpenCV_LIBS})

target_include_directories(cpp_server PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
target_compile_features(cpp_server PUBLIC c_std_99 cxx_std_17)
ament_target_dependencies(
  cpp_server
  "rclcpp"
  "std_msgs"
  "base_interfaces_demo"
)

# ---------- 客户端 ----------
add_executable(cpp_client src/cpp_client.cpp)
target_include_directories(cpp_client PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
target_compile_features(cpp_client PUBLIC c_std_99 cxx_std_17)
ament_target_dependencies(
  cpp_client
  "rclcpp"
  "std_msgs"
  "base_interfaces_demo"
)

# ---------- 图像发布者 ----------
add_executable(cpp_talker src/cpp_talker.cpp)
target_include_directories(cpp_talker PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(cpp_talker ${OpenCV_LIBS})
target_compile_features(cpp_talker PUBLIC cxx_std_17)
ament_target_dependencies(
  cpp_talker
  "rclcpp"
  "sensor_msgs"
)

install(TARGETS 
  cpp_server
  cpp_client
  cpp_talker
  DESTINATION lib/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()