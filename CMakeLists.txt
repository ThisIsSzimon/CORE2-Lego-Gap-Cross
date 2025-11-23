cmake_minimum_required(VERSION 2.8)
project(ARoboCoreProject NONE)

include("D:/Coding/LEGO/husarion.husarion-1.5.30/sdk/hFramework.cmake")

enable_module(hCloudClient)
enable_module(hSensors)
enable_module(hModules)
enable_module(hROS)

add_hexecutable(myproject main.cpp)

set_target_properties(myproject.elf PROPERTIES LINK_FLAGS "-Wl,--allow-multiple-definition")

