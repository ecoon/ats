# -*- mode: cmake -*-
include_directories(${ATS_SOURCE_DIR}/src/pks)
include_directories(${ATS_SOURCE_DIR}/src/factory)
include_directories(${ATS_SOURCE_DIR}/src/pks/flow/bc_factory)
include_directories(${ATS_SOURCE_DIR}/src/pks/flow/constitutive_relations/overland_conductivity)
include_directories(${ATS_SOURCE_DIR}/src/pks/flow/constitutive_relations/elevation)
include_directories(${ATS_SOURCE_DIR}/src/operators/divgrad)
include_directories(${ATS_SOURCE_DIR}/src/operators/divgrad/upwind_scheme)
include_directories(${ATS_SOURCE_DIR}/src/pks/flow/overland_pressure)

add_library(pk_flow_icy_overland
    icy_overland.cc
    icy_height_evaluator.cc)

install(TARGETS pk_flow_icy_overland DESTINATION lib)


