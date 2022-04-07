cmake_minimum_required(VERSION 3.21)
project(tesla_charge)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)
include_directories(src)
include_directories(src/garage)
include_directories(src/tesla-charge-interface)
include_directories(/Users/steve/Documents/Arduino/library)

#add_executable(tesla_charge
#        src/garage/garage.h
#        src/garage/garage.ino
#        src/tesla-charge-interface/myFont.h
#        src/tesla-charge-interface/tesla-charge-interface.ino
#        src/tesla-charge-interface/TeslaCharge.h src/garage_door_prototype/garage_door_prototype.ino src/garage_door_prototype/garage_door_prototype.h)
#