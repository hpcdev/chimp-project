cmake_minimum_required(VERSION 2.6)

project( GlobalPic )

find_package( LibXml2 REQUIRED )

include_directories(
  ${LIBXML2_INCLUDE_DIR}
  ${GlobalPic_SOURCE_DIR}
  /home/shane/Project/chimp-project/install/include )

add_definitions( 
  ${LIBXML2_DEFINITIONS} )
  
link_directories(
  /home/shane/Project/chimp-project/install/lib )

add_executable( GlobalPicConsole main.cpp )

target_link_libraries( GlobalPicConsole
  misc
  random
  particledb
  physical
  ${LIBXML2_LIBRARIES}
	boost_regex )

