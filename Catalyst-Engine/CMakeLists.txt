# CMakeList.txt : CMake project for Catalyst-Engine, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (Catalyst-Engine "Catalyst-Engine.cpp" "Catalyst-Engine.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Catalyst-Engine PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
