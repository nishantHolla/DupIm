
cmake_minimum_required(VERSION 3.2)

project(Dupim)

set(SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)
set(INCLUDE_DIR ${SOURCE_DIR}/include)
set(LIBRARY_DIR ${SOURCE_DIR}/lib)

include_directories(${INCLUDE_DIR})
link_directories(${LIBRARY_DIR})

add_executable(${PROJECT_NAME}
	${SOURCE_DIR}/main.cpp
)

find_package(OpenCV REQUIRED)

target_link_libraries(${PROJECT_NAME}
	sisIO
	pHash
	fftw3_omp
	fftw3
	pthread
	${OpenCV_LIBS}
)
