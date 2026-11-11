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
    obd/MonitorTests.h        obd/MonitorTests.cpp
    obd/Units.h               obd/Units.cpp
    obd/VehicleInfo.h         obd/VehicleInfo.cpp
    obd/ObdClient.h           obd/ObdClient.cpp
    obd/ModuleScanner.h       obd/ModuleScanner.cpp
    obd/ModuleAddressing.h    obd/ModuleAddressing.cpp
    obd/ManufacturerData.h    obd/ManufacturerData.cpp
    obd/ManufacturerReader.h  obd/ManufacturerReader.cpp
    vehicles/ModuleCatalog.h  vehicles/ModuleCatalog.cpp
    safety/RequestGuard.h     safety/RequestGuard.cpp
    transport/Transport.h
    transport/TcpTransport.h  transport/TcpTransport.cpp
    transport/SimulatedTransport.h transport/SimulatedTransport.cpp
    transport/ReplayTransport.h transport/ReplayTransport.cpp
    recording/Recording.h     recording/Recording.cpp
    recording/SessionRecorder.h recording/SessionRecorder.cpp
    report/VehicleReport.h    report/VehicleReport.cpp
    trip/TripComputer.h       trip/TripComputer.cpp
    chart/SignalSeries.h      chart/SignalSeries.cpp
    elm327/ElmSession.h       elm327/ElmSession.cpp
    elm327/ConnectionKeeper.h elm327/ConnectionKeeper.cpp
    elm327/AdapterCheck.h     elm327/AdapterCheck.cpp
)

target_include_directories(obdaryn_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(obdaryn_core PUBLIC Qt6::Core Qt6::Network)
obdaryn_set_warnings(obdaryn_core)

# Data files compiled into the binary: DTC descriptions, the brand module
# profiles (one JSON file per brand; drop a new file in data/vehicles to add one)
# and the manufacturer value packs imported from OBDb (CC BY-SA 4.0, see
# data/obdb/LICENSE.md).
file(GLOB OBDARYN_VEHICLE_PROFILES "${CMAKE_CURRENT_SOURCE_DIR}/data/vehicles/*.json")
file(GLOB OBDARYN_DTC_BRANDS "${CMAKE_CURRENT_SOURCE_DIR}/data/dtc/brands/*.json")
file(GLOB OBDARYN_OBDB_PACKS "${CMAKE_CURRENT_SOURCE_DIR}/data/obdb/*.json")
qt_add_resources(obdaryn_core "obdaryn_core_data"
    PREFIX "/obdaryn"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES
        data/dtc_generic.json
        data/dtc_generic_ro.json
        ${OBDARYN_VEHICLE_PROFILES}
        ${OBDARYN_DTC_BRANDS}
        ${OBDARYN_OBDB_PACKS}
)
