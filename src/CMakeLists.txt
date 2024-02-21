set(SOURCE_FIEL
  ThreadCache.cc
  CentralCache.cc
  PageCache.cc
)

add_executable(UnitTest.exe ${SOURCE_FIEL} UnitTest.cc)
target_include_directories(UnitTest.exe PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

add_executable(Benchmark.exe ${SOURCE_FIEL} Benchmark.cc)
target_include_directories(Benchmark.exe PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)