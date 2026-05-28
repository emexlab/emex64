# Makefile

all: compile

# Prepares the build directory
build/.build:
	mkdir -p build
	cd build; \
		cmake .. -G Ninja
	touch build/.build

# Builds the targets
compile: build/.build
	cd build; \
		ninja

# And install
install: compile
	cd build; \
		sudo ninja install

clean:
	rm -rf build
