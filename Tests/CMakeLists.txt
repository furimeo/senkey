add_executable(engine_test EngineTest.cpp)
target_link_libraries(engine_test PRIVATE SenKeyCoreLib UniKeyCore pthread)

add_executable(pipeline_memory_test PipelineMemoryTest.cpp)
target_link_libraries(pipeline_memory_test PRIVATE SenKeyCoreLib UniKeyCore pthread)

add_test(NAME EngineTest COMMAND engine_test)
add_test(NAME PipelineMemoryTest COMMAND pipeline_memory_test)
