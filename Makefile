# Makefile

CMAKE_FLAGS ?=

all: compile

# Prepares the build directory
rerun_cmake:
	mkdir -p build
	cd build; \
		cmake $(CMAKE_FLAGS) ..

# Builds the targets
compile: rerun_cmake
	cd build; \
		make

# And install
install: compile
	cd build; \
		sudo make install

clean:
	rm -rf build

xcode_debug: CMAKE_FLAGS = -DEMEX64_BUILD_TOOLS=1 -DEMEX64_BUILD_EXAMPLES=1 -DEMEX64LIB_STATIC=1 -DCMAKE_BUILD_TYPE="Debug"
xcode_debug:
	killall Xcode
	rm -rf build
	mkdir -p build
	cd build; \
	    cmake $(CMAKE_FLAGS) .. -G Xcode
	open build/emex64Toolchain.xcodeproj
