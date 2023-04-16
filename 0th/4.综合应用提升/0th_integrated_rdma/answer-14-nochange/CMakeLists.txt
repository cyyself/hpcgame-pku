cmake_minimum_required(VERSION 3.11)

project(rdmarb_demo)

set(CMAKE_CXX_STANDARD 17)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/Modules)

find_package(ibverbs REQUIRED)

add_compile_options(-Wall -Wextra -Wpedantic -O0 -g)

include_directories(include)

add_library(rdmarb_farm common.cpp farm.cpp)
target_link_libraries(rdmarb_farm ibverbs)




add_executable(producer farm_prod.cpp)
target_link_libraries(producer PRIVATE rdmarb_farm)

add_executable(consumer farm_cons.cpp)
target_link_libraries(consumer PRIVATE rdmarb_farm)
