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
