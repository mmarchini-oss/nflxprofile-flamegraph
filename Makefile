all: build/parse.js

build/nflxprofile.pb-c.c: deps/nflxprofile/nflxprofile.proto
	mkdir -p build/
	cd deps/nflxprofile && protoc-c --c_out=../../build/ nflxprofile.proto

build/include/nflxprofile.pb-c.h: build/nflxprofile.pb-c.c
	mkdir -p build/include
	cp build/nflxprofile.pb-c.h build/include/

build/lib/libprotobuf-c.a:
	cd deps/protobuf-c && ./autogen.sh
	cd deps/protobuf-c && emconfigure ./configure --prefix $(shell pwd)/build --disable-protoc
	emmake make -j10 -C deps/protobuf-c install


build/parse.js: parse.c build/nflxprofile.pb-c.c build/include/nflxprofile.pb-c.h deps/parson/parson.c build/lib/libprotobuf-c.a
	emcc parse.c build/nflxprofile.pb-c.c deps/parson/parson.c -o build/parse.js -lnodefs.js -I build/include -s 'EXTRA_EXPORTED_RUNTIME_METHODS=["ccall", "cwrap", "writeArrayToMemory", "AsciiToString"]' -s EXPORTED_FUNCTIONS="['_malloc', '_free']" -s MODULARIZE=1 -s 'EXPORT_NAME="NflxprofileParse"' -s ALLOW_MEMORY_GROWTH=1 -lprotobuf-c -L build/lib -O2
