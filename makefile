# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# Copyright © 2023 Rak Laptudirm <rak@laptudirm.com>                        #
#                                                                           #
# Licensed under the Apache License, Version 2.0 (the "License");           #
# you may not use this file except in compliance with the License.          #
# You may obtain a copy of the License at                                   #
# http://www.apache.org/licenses/LICENSE-2.0                                #
#                                                                           #
# Unless required by applicable law or agreed to in writing, software       #
# distributed under the License is distributed on an "AS IS" BASIS,         #
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  #
# See the License for the specific language governing permissions and       #
# limitations under the License.                                            #
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

# =================================== #
# Target Program Name and Source Path #
# =================================== #
         PROGRAM  = mess
override SRC_PATH = cmd/$(PROGRAM)

# ====================== #
# Engine Executable Name #
# ====================== #
EXE = $(PROGRAM)

# ======================= #
# Code Building Directory #
# ======================= #
override BUILD_ROOT = cmake-build
override BUILD_DIR  = $(BUILD_ROOT)/$(PROGRAM)/$(CONFIG)

CONFIG = Release

# =========================================================== #
# Compiler Selection: If CC and CXX variables are not set,    #
# clang and clang++ compilers are used by default to provide  #
# binaries with the best performance, or CC and CXX are used. #
# =========================================================== #
ifeq ($(CC),)
    CC  = clang
    CXX = clang++
endif

# ===================================================== #
# OS Specific Tasks: Setup the correct copy command and #
# executable file extension according to the target os. #
# ===================================================== #
ifeq ($(OS),Windows_NT)
    override CP = powershell cp
    override EXTENSION = .exe
else
    override CP = cp
    override EXTENSION =
endif

# ======================= #
# Default Makefile Target #
# ======================= #
.PHONY: default
default: build

# ============================ #
# All Make targets are Phonies #
# ============================ #
.PHONY: build      # Build Utilities
.PHONY: clean help # Other Utilities

# =============================================================================== #
# CMake Setup: Setup Mess's build system with the Ninja Generator, which allows   #
# incremental compilations to the source code. This is useful for developers, but #
# probably won't cause any change for users building release binaries with it.    #
# =============================================================================== #
build:
	cmake $(SRC_PATH) -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(CONFIG)   \
	-DCMAKE_C_COMPILER=$(CC) -DCMAKE_CXX_COMPILER=$(CXX) -G Ninja
	cmake --build $(BUILD_DIR) --config $(CONFIG)
	@$(CP) $(BUILD_DIR)/$(PROGRAM)$(EXTENSION) $(EXE)$(EXTENSION)

pgo-build:
	cmake $(SRC_PATH) -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(CONFIG)   \
	-DCMAKE_C_COMPILER=$(CC) -DCMAKE_CXX_COMPILER=$(CXX) -G Ninja -DPGO=True
	cmake --build $(BUILD_DIR) --config $(CONFIG)
	$(BUILD_DIR)/mess
	llvm-profdata merge -output=$(BUILD_DIR)/pgo.profdata $(BUILD_DIR)/pgo.profraw
	cmake $(SRC_PATH) -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(CONFIG)   \
	-DCMAKE_C_COMPILER=$(CC) -DCMAKE_CXX_COMPILER=$(CXX) -G Ninja -DPGO=True
	cmake --build $(BUILD_DIR) --config $(CONFIG)
	@$(CP) $(BUILD_DIR)/$(PROGRAM)$(EXTENSION) $(EXE)$(EXTENSION)

# ===================================================================== #
# Utility Commands which are not build targets but useful for the user. #
# ===================================================================== #

clean:
	@rm -rf cmake-build
	@echo "make: removed cached build information"

help:
	@echo "USAGE:"
	@echo "    for builds: make [ clean build ] { VARIABLE=VALUE }"
	@echo "    for others: make UTILITY"
	@echo
	@echo "UTILITIES:"
	@echo "    clean    remove all the cached target build information"
	@echo "    help     print this help message for ease of use"
	@echo
	@echo "VARIABLES: (VARIABLE = DEFAULT_VALUE)"
	@echo "    PROGRAM = mess              the program that will be built. possible values:"
	@echo "                                - mess: the actual chess engine"
	@echo "                                - tests: catch2 exe containing all unit tests"
	@echo
	@echo "    CONFIG  = Release           the build configuration to use. possible values:"
	@echo "                                - Debug: full debug and sanitization information"
	@echo "                                - Release: fastest and most optimized build"
	@echo "                                - RelWithDebInfo: Release + asserts"
	@echo
	@echo "    EXE     = bin/PROGRAM       the path to the built executable"
	@echo
	@echo "    C       = clang             the C   compiler used for the build"
	@echo "    CXX     = clang++           the C++ compiler used for the build"
	@echo
