# -----------------------------------------------------------------------------------
# Copyright 2024, Gilles Zunino
# -----------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

idf_build_set_property(MINIMAL_BUILD ON)
project(main-max-7219-7221)
