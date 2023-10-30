add_library(
    ${CLYPSALOT_LIB_TARGET} SHARED

    catalog.hxx catalog.cxx
    error.hxx error.cxx
    event.hxx event.cxx
    forward.hxx
    logging.hxx logging.cxx
    macros.hxx
    message.hxx message.cxx
    module.hxx module.cxx
    network.hxx network.cxx
    object.hxx object.cxx
    port.hxx port.cxx
    property.hxx property.cxx
    thread.hxx thread.cxx
    util.hxx util.cxx
)

target_link_libraries(
    ${CLYPSALOT_LIB_TARGET}

    PUBLIC Threads::Threads
    PRIVATE Boost::boost
)
