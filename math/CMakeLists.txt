# find_package(eigen3 REQUIRED)

add_library(math iforce.cpp)
target_include_directories(math PUBLIC .)
# target_link_libraries(math PRIVATE Eigen3::Eigen)


