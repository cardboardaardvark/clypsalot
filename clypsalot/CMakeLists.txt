add_library(
    ${CLYPSALOT_LIBRARY_TARGET} SHARED

    error.hxx
    error.cxx
    event.hxx
    event.cxx
    forward.hxx
    logging.hxx
    logging.cxx
    macros.hxx
    object.hxx
    object.cxx
    thread.hxx
    thread.cxx
    util.hxx
    util.cxx
)

target_link_libraries(${CLYPSALOT_LIBRARY_TARGET} ${Boost_LIBRARIES})
