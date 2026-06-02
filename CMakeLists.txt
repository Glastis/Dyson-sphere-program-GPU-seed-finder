cmake_minimum_required(VERSION 3.20)

option(DSP_CPU_REFERENCE "Build the CPU-only reference binary (no CUDA; for tests and ground truth)" OFF)

project(dsp_seed_finder C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(DSP_HOST_SOURCES
    src/main.c
    src/cli/cli.c
    src/json/conditions.c
    src/json/json_value.c
    src/output/writer.c)

set(DSP_WARNINGS -Wall -Wextra)

if(DSP_CPU_REFERENCE)
    add_executable(dsp-seed-finder-cpu ${DSP_HOST_SOURCES} src/verify/engine.c)
    target_include_directories(dsp-seed-finder-cpu PRIVATE src)
    target_compile_options(dsp-seed-finder-cpu PRIVATE ${DSP_WARNINGS})
    find_package(Threads REQUIRED)
    target_link_libraries(dsp-seed-finder-cpu PRIVATE Threads::Threads m)
    message(STATUS "Configured CPU reference build (dsp-seed-finder-cpu). No CUDA required.")
else()
    find_program(DSP_NVCC nvcc
        HINTS ENV CUDA_PATH ENV CUDA_HOME
        PATHS /usr/local/cuda /usr/local/cuda-13 /usr/local/cuda-12 /opt/cuda
        PATH_SUFFIXES bin)
    if(NOT DSP_NVCC)
        message(FATAL_ERROR
            "\n"
            "================================================================\n"
            "  nvcc (NVIDIA CUDA compiler) was not found.\n"
            "  This tool targets the RTX 5070 (Blackwell, compute capability\n"
            "  sm_120) and needs the CUDA Toolkit >= 12.8 to build the GPU\n"
            "  search engine.\n"
            "\n"
            "  Install the CUDA Toolkit (>= 12.8) so that 'nvcc' is on PATH,\n"
            "  then re-run cmake.\n"
            "\n"
            "  To build the host-only CPU reference instead (no GPU, used for\n"
            "  tests and as the verification ground truth), configure with:\n"
            "      cmake -DDSP_CPU_REFERENCE=ON ..\n"
            "================================================================\n")
    endif()
    set(CMAKE_CUDA_COMPILER "${DSP_NVCC}")
    enable_language(CUDA)
    set(CMAKE_CUDA_STANDARD 14)
    set(CMAKE_CUDA_ARCHITECTURES 120)
    add_executable(dsp-seed-finder
        ${DSP_HOST_SOURCES}
        src/gpu/gpu_search.c
        src/gpu/gpu_kernel.cu)
    target_include_directories(dsp-seed-finder PRIVATE src)
    target_compile_options(dsp-seed-finder PRIVATE
        $<$<COMPILE_LANGUAGE:C>:${DSP_WARNINGS}>)
    find_package(Threads REQUIRED)
    target_link_libraries(dsp-seed-finder PRIVATE Threads::Threads m)
    message(STATUS "Configured GPU build (dsp-seed-finder) for sm_120 with ${DSP_NVCC}.")
endif()
