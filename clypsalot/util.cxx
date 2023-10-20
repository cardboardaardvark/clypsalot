#include <boost/core/demangle.hpp>

#include <clypsalot/util.hxx>

/// @file
namespace Clypsalot
{
    /// @brief Return a demangled name of a type.
    std::string typeName(const std::type_info& type)
    {
        return boost::core::demangle(type.name());
    }
}
