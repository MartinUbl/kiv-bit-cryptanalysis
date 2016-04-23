FILE(GLOB tmp_src
    *.h
    *.cpp
    *.c
)

ADD_EXECUTABLE(kiv-bit-cryptanalysis-worker ${tmp_src})

IF(CMAKE_COMPILER_IS_GNUCXX)
    TARGET_LINK_LIBRARIES(kiv-bit-cryptanalysis-worker pthread m)
ENDIF()
