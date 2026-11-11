# SPDX-License-Identifier: GPL-3.0-or-later
#
# Unit tests. They never need an adapter or a car: protocol-level tests feed
# recorded text into the parsers, and session tests run against the built-in
# SimulatedTransport. Run with:  ctest --test-dir build --output-on-failure

function(obdaryn_add_test name)
    qt_add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE obdaryn_core Qt6::Test)
    obdaryn_set_warnings(${name})
    add_test(NAME ${name} COMMAND ${name})
endfunction()

obdaryn_add_test(tst_requestguard)
obdaryn_add_test(tst_responseparser)
obdaryn_add_test(tst_decoders)
obdaryn_add_test(tst_session)
obdaryn_add_test(tst_modules)
obdaryn_add_test(tst_recording)
obdaryn_add_test(tst_report)
obdaryn_add_test(tst_trip)
obdaryn_add_test(tst_health)
obdaryn_add_test(tst_mode06)
obdaryn_add_test(tst_charts)
obdaryn_add_test(tst_manufacturer)
# Recorded answers from real cars (OBDb test cases, CC BY-SA 4.0).
target_compile_definitions(tst_manufacturer PRIVATE OBDARYN_TEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}/data")
