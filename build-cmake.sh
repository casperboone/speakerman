#!/usr/bin/dash
cmake -DCMAKE_BUILD_TYPE=MinSizeRel -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang -DCMAKE_DEPENDS_USE_COMPILER=FALSE -G "CodeBlocks - Unix Makefiles" -S ./ -B ./build-minsizerel