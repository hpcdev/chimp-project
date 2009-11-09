cmake_minimum_required(VERSION 2.6)

project( mccDriver )

find_package( LibXml2 REQUIRED )

include_directories(
  ${LIBXML2_INCLUDE_DIR}
  ${mccDriver_SOURCE_DIR}
  /home/shane/Project/chimp-project/install/include )

add_definitions( 
  ${LIBXML2_DEFINITIONS} )
  
link_directories(
  /home/shane/Project/chimp-project/install/lib )

add_executable( mccDriverConsole main.cpp )

target_link_libraries( mccDriverConsole
  misc
  random
  particledb
  physical
  ${LIBXML2_LIBRARIES}
	boost_regex )

