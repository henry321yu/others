cmake_minimum_required(VERSION 3.10)

# 設定專案名稱
project(lidar)

# 設定 C++ 標準
set(CMAKE_CXX_STANDARD 14)

# 指定 Boost 路徑
set(BOOST_ROOT "C:/Users/sgrc-325/vcpkg/installed/x64-windows")

# 查找 Boost
find_package(Boost REQUIRED COMPONENTS system)

# 指定包含目錄
include_directories("${BOOST_ROOT}/include")

# 指定庫目錄
link_directories("${BOOST_ROOT}/lib")

# 定義你的執行檔
add_executable(lidar main.cpp)

# 連結 Boost 系統庫
target_link_libraries(lidar Boost::system ws2_32)