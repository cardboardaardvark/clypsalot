/* Copyright 2023 Tyler Riddle
 *
 * This file is part of Clypsalot. Clypsalot is free software: you can redistribute it and/or modify it under the terms
 * of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version. Clypsalot is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with Clypsalot. If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include <cassert>

#include <clypsalot/error.hxx>
#include <clypsalot/macros.hxx>
#include <clypsalot/property.hxx>
#include <clypsalot/thread.hxx>
#include <clypsalot/util.hxx>

namespace Clypsalot
{
    Property::Property(const Lockable& in_parent, const PropertyConfig& in_config) :
        m_parent(in_parent),
        m_container(),
        m_name(in_config.name),
        m_type(in_config.type),
        m_flags(in_config.flags)
    {
        switch (in_config.type)
        {
            case PropertyType::boolean: m_container.boolean = false; break;
            case PropertyType::file: m_container.file = new std::filesystem::path; break;
            case PropertyType::integer: m_container.integer = 0; break;
            case PropertyType::real: m_container.real = 0.; break;
            case PropertyType::size: m_container.size = 0; break;
            case PropertyType::string: m_container.string = new std::string; break;
        }

        if (in_config.initial.type() != typeid(nullptr))
        {
            set(in_config.initial);
        }
    }

    Property::~Property()
    {
        if (m_type == PropertyType::string)
        {
            delete m_container.string;
        }
        else if (m_type == PropertyType::file)
        {
            delete m_container.file;
        }
    }

    const std::string& Property::name() const noexcept
    {
        return m_name;
    }

    PropertyType Property::type() const noexcept
    {
        return m_type;
    }

    Property::Flags Property::flags() const noexcept
    {
        return m_flags;
    }

    bool Property::hasFlag(const Property::Flags in_flags) const noexcept
    {
        return m_flags & in_flags;
    }

    void Property::defined(const bool in_defined)
    {
        assert(m_parent.haveLock());

        m_hasValue = in_defined;
    }

    bool Property::defined() const noexcept
    {
        assert(m_parent.haveLock());

        return m_hasValue;
    }

    void Property::set(const std::any& in_value)
    {
        assert(m_parent.haveLock());

        switch (m_type)
        {
            case PropertyType::boolean: m_container.boolean = anyToBool(in_value); break;
            case PropertyType::file: *m_container.file = anyToPath(in_value); break;
            case PropertyType::integer: m_container.integer = anyToInt(in_value); break;
            case PropertyType::real: m_container.real = anyToFloat(in_value); break;
            case PropertyType::size: m_container.size = anyToSize(in_value); break;
            case PropertyType::string: *m_container.string = anyToString(in_value); break;
        }

        m_hasValue = true;
    }

    void Property::enforcePublicMutable() const
    {
        if (! (m_flags & PublicMutable)) throw ImmutableError(makeString("Property ", m_name, " is not mutable"));
    }

    void Property::enforceType(const PropertyType in_enforceType) const
    {
        if (m_type != in_enforceType) throw TypeError(makeString("Property ", m_name, " is not of ", in_enforceType, " type"));
    }

    void Property::enforceDefined() const
    {
        if (! m_hasValue) throw UndefinedError(makeString("Property ", m_name, " does not have a value"));
    }

    std::string Property::valueToString() const
    {
        assert(m_parent.haveLock());

        switch (m_type)
        {
            case PropertyType::boolean: return makeString(booleanValue());
            case PropertyType::file: return fileValue().string();
            case PropertyType::integer: return makeString(integerValue());
            case PropertyType::real: return makeString(realValue());
            case PropertyType::size: return makeString(sizeValue());
            case PropertyType::string: return stringValue();
        }

        FATAL_ERROR(makeString("Unhandled PropertyType value: ", m_type));
    }

    Property::BooleanType& Property::booleanRef()
    {
        assert(m_parent.haveLock());

        enforceType(PropertyType::boolean);

        return m_container.boolean;
    }

    Property::BooleanType Property::booleanValue() const
    {
        assert(m_parent.haveLock());

        enforceType(PropertyType::boolean);
        enforceDefined();

        return m_container.boolean;
    }

    void Property::booleanValue(const BooleanType in_value)
    {
        assert(m_parent.haveLock());

        enforcePublicMutable();

        booleanRef() = in_value;
        m_hasValue = true;
    }

    Property::FileType& Property::fileRef()
    {
        assert(m_parent.haveLock());

        enforceType(PropertyType::file);

        return *m_container.file;
    }

    Property::FileType Property::fileValue() const
    {
        assert(m_parent.haveLock());

        enforceType(PropertyType::file);
        enforceDefined();

        return *m_container.file;
    }

    void Property::fileValue(const FileType& in_value)
    {
        assert(m_parent.haveLock());

        enforcePublicMutable();

        fileRef() = in_value;
        m_hasValue = true;
    }

    Property::IntegerType& Property::integerRef()
    {
        assert(m_parent.haveLock());

        enforceType(PropertyType::integer);

        return m_container.integer;
    }

    Property::IntegerType Property::integerValue() const
    {
        assert(m_parent.haveLock());

        enforceType(PropertyType::integer);
        enforceDefined();

        return m_container.integer;
    }

    void Property::integerValue(const IntegerType in_value)
    {
        assert(m_parent.haveLock());

        enforcePublicMutable();

        integerRef() = in_value;
        m_hasValue = true;
    }

    Property::RealType& Property::realRef()
    {
        assert(m_parent.haveLock());

        enforceType(PropertyType::real);

        return m_container.real;
    }

    Property::RealType Property::realValue() const
    {
        assert(m_parent.haveLock());

        enforceType(PropertyType::real);
        enforceDefined();

        return m_container.real;
    }

    void Property::realValue(const RealType in_value)
    {
        assert(m_parent.haveLock());

        enforcePublicMutable();

        realRef() = in_value;
        m_hasValue = true;
    }

    Property::SizeType& Property::sizeRef()
    {
        assert(m_parent.haveLock());

        enforceType(PropertyType::size);

        return m_container.size;
    }

    Property::SizeType Property::sizeValue() const
    {
        assert(m_parent.haveLock());

        enforceType(PropertyType::size);
        enforceDefined();

        return m_container.size;
    }

    void Property::sizeValue(const SizeType in_value)
    {
        assert(m_parent.haveLock());

        enforcePublicMutable();

        sizeRef() = in_value;
        m_hasValue = true;
    }

    Property::StringType& Property::stringRef()
    {
        assert(m_parent.haveLock());

        enforceType(PropertyType::string);

        return *m_container.string;
    }

    Property::StringType Property::stringValue() const
    {
        assert(m_parent.haveLock());

        enforceType(PropertyType::string);
        enforceDefined();

        return *m_container.string;
    }

    void Property::stringValue(const StringType& in_value)
    {
        assert(m_parent.haveLock());

        enforcePublicMutable();

        stringRef() = in_value;
        m_hasValue = true;
    }

    std::any Property::anyValue() const
    {
        if (! m_hasValue) return std::any(nullptr);

        switch (m_type)
        {
            case PropertyType::boolean: return std::any(booleanValue());
            case PropertyType::file: return std::any(fileValue());
            case PropertyType::integer: return std::any(integerValue());
            case PropertyType::real: return std::any(realValue());
            case PropertyType::size: return std::any(sizeValue());
            case PropertyType::string: return std::any(stringValue());
        }

        FATAL_ERROR(makeString("Unhandled PropertyType value: ", m_type));
    }

    void Property::anyValue(const std::any& in_value)
    {
        assert(m_parent.haveLock());

        enforcePublicMutable();

        set(in_value);
    }

    std::string toString(const PropertyType in_type) noexcept
    {
        switch (in_type)
        {
            case PropertyType::boolean: return "boolean";
            case PropertyType::file: return "file";
            case PropertyType::integer: return "integer";
            case PropertyType::real: return "real";
            case PropertyType::size: return "size";
            case PropertyType::string: return "string";
        }

        FATAL_ERROR(makeString("Unhandled PropertyType value: ", static_cast<int>(in_type)));
    }

    std::ostream& operator<<(std::ostream& in_os, const PropertyType& in_rhs) noexcept
    {
        in_os << toString(in_rhs);
        return in_os;
    }
}
