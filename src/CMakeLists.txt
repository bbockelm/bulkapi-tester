
ROOT_GENERATE_DICTIONARY(G__SillyStruct SillyStruct.h LINKDEF SillyStructLinkDef.h)
add_library(SillyStruct SHARED SillyStruct.cxx G__SillyStruct.cxx)
target_link_libraries(SillyStruct ${ROOT_LIBRARIES})
add_executable(testSillyStruct MainSillyStruct.cxx)
target_link_libraries(testSillyStruct SillyStruct)

add_executable(floatMicroBenchmark FloatMicroBenchmark.cxx)
add_executable(floatFastReaderMicroBenchmark FloatFastReaderMicroBenchmark.cxx)
target_link_libraries(floatMicroBenchmark ${ROOT_LIBRARIES})
target_link_libraries(floatFastReaderMicroBenchmark ${ROOT_LIBRARIES})

