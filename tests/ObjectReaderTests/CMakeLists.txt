
add_executable(ObjectReaderTest ObjectReaderTest.cpp)
target_link_libraries(ObjectReaderTest gtest_main gtest gmock)
add_test(NAME ObjectReaderTest COMMAND ObjectReaderTest)

