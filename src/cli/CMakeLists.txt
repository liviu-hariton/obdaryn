# SPDX-License-Identifier: GPL-3.0-or-later
# obdaryn-probe: command-line adapter/car check built on obdaryn_core.
qt_add_executable(obdaryn-probe main.cpp)
target_link_libraries(obdaryn-probe PRIVATE obdaryn_core)
obdaryn_set_warnings(obdaryn-probe)
