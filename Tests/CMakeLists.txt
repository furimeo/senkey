add_executable(engine_test EngineTest.cpp)
target_link_libraries(engine_test PRIVATE SenKeyCoreLib UniKeyCore pthread)

add_executable(pipeline_memory_test PipelineMemoryTest.cpp)
target_link_libraries(pipeline_memory_test PRIVATE SenKeyCoreLib UniKeyCore pthread)

add_executable(pipeline_queue_test PipelineQueueTest.cpp)
target_link_libraries(pipeline_queue_test PRIVATE SenKeyCoreLib UniKeyCore pthread)

add_test(NAME EngineTest COMMAND engine_test)
add_test(NAME PipelineMemoryTest COMMAND pipeline_memory_test)
add_test(NAME PipelineQueueTest COMMAND pipeline_queue_test)

if(X11_FOUND)
    add_executable(clipboard_bridge_test ClipboardBridgeTest.cpp)
    target_link_libraries(clipboard_bridge_test PRIVATE SenKeyCoreLib UniKeyCore pthread)
    add_test(NAME ClipboardBridgeTest COMMAND clipboard_bridge_test)
endif()

add_executable(wlr_virtual_keyboard_test WlrVirtualKeyboardTest.cpp)
target_link_libraries(wlr_virtual_keyboard_test PRIVATE SenKeyCoreLib UniKeyCore pthread)
if(XKBCOMMON_FOUND)
    target_link_libraries(wlr_virtual_keyboard_test PRIVATE ${XKBCOMMON_LIBRARIES})
endif()
if(WAYLAND_CLIENT_FOUND)
    target_link_libraries(wlr_virtual_keyboard_test PRIVATE ${WAYLAND_CLIENT_LIBRARIES})
endif()
add_test(NAME WlrVirtualKeyboardTest COMMAND wlr_virtual_keyboard_test)
