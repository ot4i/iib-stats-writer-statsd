cmake_minimum_required (VERSION 3.5)

include_directories(..)

include(../conanbuildinfo.cmake)
conan_basic_setup()

set (Boost_USE_STATIC_LIBS ON)
find_package (Boost COMPONENTS system)
if (NOT Boost_FOUND)
  message (FATAL_ERROR "Could not find Boost!")
endif ()
include_directories (${Boost_INCLUDE_DIRS})

if (NOT IS_DIRECTORY ${IIB_INSTALL_DIR})
  message (FATAL_ERROR "You must set the IIB_INSTALL_DIR variable!")
endif ()
set (IIB_INCLUDES_DIR ${IIB_INSTALL_DIR}/server/include/plugin)
if (NOT IS_DIRECTORY ${IIB_INCLUDES_DIR})
  message (FATAL_ERROR "The directory ${IIB_INCLUDES_DIR} does not exist!")
endif ()
set (IIB_LIBRARIES_DIR ${IIB_INSTALL_DIR}/server/lib)
if (NOT IS_DIRECTORY ${IIB_LIBRARIES_DIR})
  message (FATAL_ERROR "The directory ${IIB_LIBRARIES_DIR} does not exist!")
endif ()

add_definitions (-DBIP_CXX11_SUPPORT=1)
include_directories (${IIB_INCLUDES_DIR})


# Enable testing

# Linking to a .lil file (which is what IIB requires) is complicated, and for this size
# of project it's easier to build the source again.
add_executable(statsd_test test_main.cpp StatsdStatsWriter_UnitTest.cpp ../StatsdStatsWriter.cpp ../StatsdStatsWriter.hpp ../UdpSocket.cpp ../UdpSocket.hpp)
target_link_libraries (statsd_test ${Boost_LIBRARIES} gmock pthread)
set_target_properties (statsd_test PROPERTIES CXX_STANDARD 11)

enable_testing()
add_test(NAME statsd_test COMMAND statsd_test)
