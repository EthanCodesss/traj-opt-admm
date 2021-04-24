#LIBRARIES

#EIGEN3
IF(DEFINED ENV{EIGEN3_INCLUDE_DIR})
  MESSAGE(STATUS "Found Custom EIGEN3 @ $ENV{EIGEN3_INCLUDE_DIR}")
  INCLUDE_DIRECTORIES($ENV{EIGEN3_INCLUDE_DIR})
ELSE()
  FIND_PACKAGE(Eigen3 QUIET)
  IF(EIGEN3_FOUND)
    INCLUDE_DIRECTORIES(${EIGEN3_INCLUDE_DIR})
    MESSAGE(STATUS "Found EIGEN3 @ ${EIGEN3_INCLUDE_DIR}")
  ELSE(EIGEN3_FOUND)
    MESSAGE(WARNING "Cannot find EIGEN3, using local version!")
    SET(EIGEN3_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/lib/eigen3)
    INCLUDE_DIRECTORIES(${EIGEN3_INCLUDE_DIR})
  ENDIF(EIGEN3_FOUND)
ENDIF()

#BOOST
SET(Boost_USE_MULTITHREADED ON)
FIND_PACKAGE(Boost QUIET REQUIRED filesystem system iostreams timer chrono random)
IF(Boost_FOUND)
  INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
  MESSAGE(STATUS "Found BOOST @ ${Boost_INCLUDE_DIR}")
  LIST(APPEND ALL_LIBRARIES ${Boost_LIBRARIES})
  LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
ELSE(Boost_FOUND)
  MESSAGE(SEND_ERROR "Cannot find BOOST!")
  MESSAGE(STATUS "Error finding boost library: ${_boost_RELEASE_NAMES}")
ENDIF(Boost_FOUND)

#LIBIGL
option(LIBIGL_WITH_OPENGL            "Use OpenGL"         ON)
option(LIBIGL_WITH_OPENGL_GLFW       "Use GLFW"           ON)


find_package(LIBIGL REQUIRED QUIET)

#OMPL
find_package(ompl)
INCLUDE_DIRECTORIES(${OMPL_INCLUDE_DIRS})
LIST(APPEND ALL_LIBRARIES ${OMPL_LIBRARIES})

