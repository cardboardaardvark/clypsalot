add_library(
    ${CLYPSALOT_LIB_TARGET} SHARED

    catalog.hxx catalog.cxx
    error.hxx error.cxx
    event.hxx event.cxx
    forward.hxx
    logging.hxx logging.cxx
    macros.hxx
    module.hxx module.cxx
    object.hxx object.cxx
    port.hxx port.cxx
    property.hxx property.cxx
    thread.hxx thread.cxx
    util.hxx util.cxx
)

target_link_libraries(${CLYPSALOT_LIB_TARGET} PRIVATE Boost::boost)
