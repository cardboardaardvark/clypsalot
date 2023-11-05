#include <cstdlib>

#include <boost/core/demangle.hpp>

#include <clypsalot/error.hxx>
#include <clypsalot/util.hxx>

// This macro isn't very efficient but these conversions don't happen a lot
#define RETURN_ANY(any, typeName)\
    if (any.type() == typeid(typeName)) return std::any_cast<typeName>(any);\
    if (any.type() == typeid(const typeName)) return std::any_cast<const typeName>(any);

/// @file
namespace Clypsalot
{
    [[noreturn]] static void anyConversionError(const std::any& value, const std::string& type)
    {
        throw ValueError(makeString("Can't convert ", typeName(value.type()), " to a ", type));
    }

    static void enforceValue(const std::any& value)
    {
        if (! value.has_value())
        {
            throw ValueError("Undefined value");
        }
    }

    Finally::Finally(const Function& finally) :
        finally(finally)
    { }

    Finally::~Finally()
    {
        finally();
    }

    /// @brief Return a demangled name of a type.
    std::string typeName(const std::type_info& type)
    {
        return boost::core::demangle(type.name());
    }

    bool anyIsStringType(const std::any& value)
    {
        enforceValue(value);

        const auto& type = value.type();

        if (type == typeid(std::string)) return true;
        if (type == typeid(const std::string)) return true;
        if (type == typeid(std::string&)) return true;
        if (type == typeid(const std::string&)) return true;
        if (type == typeid(char *)) return true;
        if (type == typeid(const char *)) return true;

        return false;
    }

    bool anyToBool(const std::any& value)
    {
        enforceValue(value);

        if (typeid(value) == typeid(nullptr)) return false;
        RETURN_ANY(value, bool);
        RETURN_ANY(value, int);
        RETURN_ANY(value, unsigned long);
        RETURN_ANY(value, float);
        RETURN_ANY(value, double);

        if (anyIsStringType(value))
        {
            return stringToBool(anyToString(value));
        }

        anyConversionError(value, "boolean");
    }

    float anyToFloat(const std::any& value)
    {
        enforceValue(value);

        RETURN_ANY(value, float);
        RETURN_ANY(value, double);
        RETURN_ANY(value, int);
        RETURN_ANY(value, unsigned long);

        if (anyIsStringType(value))
        {
            return stringToFloat(anyToString(value));
        }

        anyConversionError(value, "float");
    }

    int anyToInt(const std::any& value)
    {
        enforceValue(value);

        RETURN_ANY(value, float);
        RETURN_ANY(value, double);
        RETURN_ANY(value, int);
        RETURN_ANY(value, unsigned long);

        if (anyIsStringType(value))
        {
            return stringToInt(anyToString(value));
        }

        anyConversionError(value, "integer");
    }

    size_t anyToSize(const std::any& value)
    {
        enforceValue(value);

        RETURN_ANY(value, float);
        RETURN_ANY(value, double);
        RETURN_ANY(value, int);
        RETURN_ANY(value, unsigned long);

        if (anyIsStringType(value))
        {
            return stringToSize(anyToString(value));
        }

        anyConversionError(value, "size");
    }

    std::string anyToString(const std::any& value)
    {
        enforceValue(value);

        RETURN_ANY(value, std::string);
        RETURN_ANY(value, char *);

        anyConversionError(value, "string");
    }

    std::filesystem::path anyToPath(const std::any& value)
    {
        enforceValue(value);

        RETURN_ANY(value, std::filesystem::path);
        RETURN_ANY(value, std::string);
        RETURN_ANY(value, char *);

        anyConversionError(value, "file");
    }

    bool stringToBool(std::string value)
    {
        std::transform(value.cbegin(), value.cend(), value.begin(), [](const unsigned char c) { return std::tolower(c); });

        if (value == "true") return true;
        if (value == "yes") return true;
        if (value == "1") return true;

        if (value == "false") return false;
        if (value == "no") return false;
        if (value == "0") return false;

        throw ValueError(makeString("Could not convert to boolean value: ", value));
    }

    float stringToFloat(const std::string& value)
    {
        return std::atof(value.c_str());
    }

    int stringToInt(const std::string& value)
    {
        return std::atoi(value.c_str());
    }

    size_t stringToSize(const std::string& value)
    {
        auto given = std::atoi(value.c_str());

        if (given < 0)
        {
            throw ValueError(makeString("Can't convert to size: ", value));
        }

        return given;
    }
}
