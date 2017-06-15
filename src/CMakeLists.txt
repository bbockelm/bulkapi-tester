
include_directories(.)
ROOT_GENERATE_DICTIONARY(G__SillyStruct SillyStruct.h LINKDEF SillyStructLinkDef.h)
add_library(SillyStruct SHARED SillyStruct.cxx G__SillyStruct.cxx)
target_link_libraries(SillyStruct ${ROOT_LIBRARIES})
add_executable(testSillyStruct MainSillyStruct.cxx)
target_link_libraries(testSillyStruct SillyStruct)

ROOT_GENERATE_DICTIONARY(G__VariableLengthStruct VariableLengthStruct.h LINKDEF VariableLengthStructLinkDef.h)
add_library(VariableLengthStruct SHARED VariableLengthStruct.cxx G__VariableLengthStruct.cxx)
target_link_libraries(VariableLengthStruct ${ROOT_LIBRARIES})
add_executable(variableFloatMicroBenchmark VariableFloatMicroBenchmark.cxx)
target_link_libraries(variableFloatMicroBenchmark VariableLengthStruct)

# Simple data object benchmarks - no dictionaries needed.
add_executable(floatMicroBenchmark FloatMicroBenchmark.cxx)
add_executable(floatDoubleMicroBenchmark FloatDoubleMicroBenchmark.cxx)
target_link_libraries(floatMicroBenchmark ${ROOT_LIBRARIES})
target_link_libraries(floatDoubleMicroBenchmark ${ROOT_LIBRARIES})

