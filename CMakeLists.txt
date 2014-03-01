cmake_minimum_required(VERSION 2.6)
project(fftproj)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

find_package(Qt4 REQUIRED)
set(CMAKE_AUTOMOC OFF)
SET(QT_USE_QTMULTIMEDIA TRUE)
include(${QT_USE_FILE})

add_library(clhelpers
	clhelpers/align.cpp
	clhelpers/check_err.cpp
	clhelpers/command_queue.cpp
	clhelpers/context.cpp
	clhelpers/device.cpp
	clhelpers/event.cpp
	clhelpers/kernel.cpp
	clhelpers/nvidia_cache.cpp
	clhelpers/platform.cpp
	clhelpers/program.cpp
)

set(CL_LIBRARIES OpenCL clhelpers)

# Gestisce -DCLFFT_SOURCE_DIR=/path/to/clFFT/src (deve puntare alla cartella "src")
if(CLFFT_SOURCE_DIR)
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(BUILD64 ON)
	else(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(BUILD64 OFF)
	endif(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(CLFFT_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/clFFT)
	add_subdirectory(${CLFFT_SOURCE_DIR} ${CLFFT_BUILD_DIR} EXCLUDE_FROM_ALL)
	include_directories(${CLFFT_SOURCE_DIR}/include ${CLFFT_BUILD_DIR}/include)
	message(STATUS "Uso clFFT in ${CLFFT_SOURCE_DIR}")

	add_definitions(-DHAVE_clFFT)
	add_executable(test-clFFT dft-tests/test-cl.cpp)
	set_target_properties(test-clFFT PROPERTIES COMPILE_FLAGS "-DALGOCLASS=clFFT")
	set(CL_LIBRARIES ${CL_LIBRARIES} clFFT)
	target_link_libraries(test-clFFT ${CL_LIBRARIES})
else(CLFFT_SOURCE_DIR)
	message(WARNING "CLFFT_SOURCE_DIR non specificata, il modulo clFFT non verra' compilato")
endif(CLFFT_SOURCE_DIR)

add_executable(ram_speed tools/ram_speed.cpp)

add_executable(cl_speed tools/cl_speed.cpp)
target_link_libraries(cl_speed ${CL_LIBRARIES})

add_executable(access_patterns tools/access_patterns.cpp)
target_link_libraries(access_patterns ${CL_LIBRARIES})

add_executable(triangle_grid tools/triangle_grid.cpp)
target_link_libraries(triangle_grid ${CL_LIBRARIES})

add_executable(test-serial_naive_dft dft-tests/test-serial.cpp)
set_target_properties(test-serial_naive_dft PROPERTIES COMPILE_FLAGS "-DALGOFUNC=serial_naive_dft -DALLOW_NPOT")

add_executable(test-serial_recursive_fft dft-tests/test-serial.cpp)
set_target_properties(test-serial_recursive_fft PROPERTIES COMPILE_FLAGS "-DALGOFUNC=serial_recursive_fft -DENABLE_CHECK")

add_executable(test-serial_nonrecursive_fft dft-tests/test-serial.cpp)
set_target_properties(test-serial_nonrecursive_fft PROPERTIES COMPILE_FLAGS "-DALGOFUNC=serial_nonrecursive_fft -DENABLE_CHECK")

add_executable(test-cl_naive_dft dft-tests/test-cl.cpp)
set_target_properties(test-cl_naive_dft PROPERTIES COMPILE_FLAGS "-DALGOCLASS=cl_naive_dft -DALLOW_NPOT")
target_link_libraries(test-cl_naive_dft ${CL_LIBRARIES})

add_executable(test-cl_dft_matrix dft-tests/test-cl.cpp)
set_target_properties(test-cl_dft_matrix PROPERTIES COMPILE_FLAGS "-DALGOCLASS=cl_dft_matrix -DALLOW_NPOT")
target_link_libraries(test-cl_dft_matrix ${CL_LIBRARIES})

add_executable(test-cl_dft_matrix_tex dft-tests/test-cl.cpp)
set_target_properties(test-cl_dft_matrix_tex PROPERTIES COMPILE_FLAGS "-DALGOCLASS=cl_dft_matrix_tex -DALLOW_NPOT")
target_link_libraries(test-cl_dft_matrix_tex ${CL_LIBRARIES})

add_executable(test-cl_dft_symmatrix dft-tests/test-cl.cpp)
set_target_properties(test-cl_dft_symmatrix PROPERTIES COMPILE_FLAGS "-DALGOCLASS=cl_dft_symmatrix -DALLOW_NPOT")
target_link_libraries(test-cl_dft_symmatrix ${CL_LIBRARIES})

# Attiva automoc per il modulo tuner
set(CMAKE_AUTOMOC ON)

qt4_wrap_ui(tuner_UI_SRCS tuner/MainWindow.ui)
add_executable(tuner
	tuner/main.cpp
	tuner/LiveAudioInput.cpp
	tuner/MainWindow.cpp
	tuner/SetupDialog.cpp
	tuner/SpectrumAnalyzer.cpp
	tuner/SpectrumWidget.cpp
	tuner/TunesWidget.cpp

	${tuner_UI_SRCS}

	dft-interface/CLAlgorithm.cpp
	dft-interface/DftAlgorithm.cpp
	dft-interface/DftAlgorithmManager.cpp
	dft-interface/SerialAlgorithm.cpp
)
target_link_libraries(tuner ${CL_LIBRARIES} ${QT_LIBRARIES})
