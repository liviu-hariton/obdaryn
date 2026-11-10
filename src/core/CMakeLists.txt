# SPDX-License-Identifier: GPL-3.0-or-later
#
# obdaryn_core — everything that talks to the adapter and understands OBD-II.
# No GUI code lives here, so the same library can back a desktop app, a mobile
# app or a command-line tool.

qt_add_library(obdaryn_core STATIC
    obd/Protocol.h            obd/Protocol.cpp
    obd/ResponseParser.h      obd/ResponseParser.cpp
    obd/Pids.h                obd/Pids.cpp
    obd/Dtc.h                 obd/Dtc.cpp
    obd/DtcDatabase.h         obd/DtcDatabase.cpp
    obd/Readiness.h           obd/Readiness.cpp
    obd/VehicleInfo.h         obd/VehicleInfo.cpp
    obd/ObdClient.h           obd/ObdClient.cpp
    obd/ModuleScanner.h       obd/ModuleScanner.cpp
    vehicles/ModuleCatalog.h  vehicles/ModuleCatalog.cpp
    safety/RequestGuard.h     safety/RequestGuard.cpp
    transport/Transport.h
    transport/TcpTransport.h  transport/TcpTransport.cpp
    transport/SimulatedTransport.h transport/SimulatedTransport.cpp
    elm327/ElmSession.h       elm327/ElmSession.cpp
)

target_include_directories(obdaryn_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(obdaryn_core PUBLIC Qt6::Core Qt6::Network)
obdaryn_set_warnings(obdaryn_core)

# Data files compiled into the binary: DTC descriptions and the brand module
# profiles (one JSON file per brand; drop a new file in data/vehicles to add one).
file(GLOB OBDARYN_VEHICLE_PROFILES "${CMAKE_CURRENT_SOURCE_DIR}/data/vehicles/*.json")
file(GLOB OBDARYN_DTC_BRANDS "${CMAKE_CURRENT_SOURCE_DIR}/data/dtc/brands/*.json")
qt_add_resources(obdaryn_core "obdaryn_core_data"
    PREFIX "/obdaryn"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES
        data/dtc_generic.json
        data/dtc_generic_ro.json
        ${OBDARYN_VEHICLE_PROFILES}
        ${OBDARYN_DTC_BRANDS}
)
