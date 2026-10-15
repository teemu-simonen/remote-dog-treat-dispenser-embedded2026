<<<<<<< HEAD
#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(peripheral_lbs)

# NORDIC SDK APP START
target_sources(app PRIVATE
  src/main.c
)

# NORDIC SDK APP END
=======
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(blink_test)

target_sources(app PRIVATE src/main.c)
>>>>>>> 8ac7c7586f60419b73b2e1f6d33a28ed7058a853
