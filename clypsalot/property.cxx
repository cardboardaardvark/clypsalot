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
#include <clypsalot/property.hxx>
#include <clypsalot/thread.hxx>
#include <clypsalot/util.hxx>

namespace Clypsalot
{
    Property::Property(
            const Lockable& parent,
            const std::string& name,
            const PropertyType type,
            const bool configurable,
            const bool required,
            const bool publicMutable,
            const std::any& initial
        ) :
            parent(parent),
            name(name),
            type(type),
            configurable(configurable),
            required(required),
            publicMutable(publicMutable)
    {
        switch (type)
        {
            case PropertyType::boolean: container.boolean = false; break;
            case PropertyType::file: container.file = new std::filesystem::path; break;
            case PropertyType::integer: container.integer = 0; break;
            case PropertyType::real: container.real = 0.; break;
            case PropertyType::size: container.size = 0; break;
            case PropertyType::string: container.string = new std::string; break;
        }

        if (initial.type() != typeid(nullptr))
        {
            set(initial);
        }
    }

    Property::~Property()
    {
        if (type == PropertyType::string)
        {
            delete container.string;
        }
        else if (type == PropertyType::file)
        {
            delete container.file;
        }
    }

    bool Property::defined(const bool defined)
    {
        assert(parent.haveLock());

        hasValue = defined;

        return hasValue;
    }

    bool Property::defined() const
    {
        assert(parent.haveLock());

        return hasValue;
    }

    void Property::set(const std::any& value)
    {
        assert(parent.haveLock());

        switch (type)
        {
            case PropertyType::boolean: container.boolean = anyToBool(value); break;
            case PropertyType::file: *container.file = anyToPath(value); break;
            case PropertyType::integer: container.integer = anyToInt(value); break;
            case PropertyType::real: container.real = anyToFloat(value); break;
            case PropertyType::size: container.size = anyToSize(value); break;
            case PropertyType::string: *container.string = anyToString(value); break;
        }

        hasValue = true;
    }

    void Property::enforcePublicMutable() const
    {
        if (! publicMutable) throw ImmutableError(makeString("Property ", name, " is not mutable"));
    }

    void Property::enforceType(const PropertyType enforceType) const
    {
        if (type != enforceType) throw TypeError(makeString("Property ", name, " is not of ", enforceType, " type"));
    }

    void Property::enforceDefined() const
    {
        if (! hasValue) throw UndefinedError(makeString("Property ", name, " does not have a value"));
    }

    std::string Property::asString() const
    {
        assert(parent.haveLock());

        switch (type)
        {
            case PropertyType::boolean: return makeString(booleanValue());
            case PropertyType::file: return fileValue();
            case PropertyType::integer: return makeString(integerValue());
            case PropertyType::real: return makeString(realValue());
            case PropertyType::size: return makeString(sizeValue());
            case PropertyType::string: return stringValue();
        }
    }

    Property::BooleanType& Property::booleanRef()
    {
        assert(parent.haveLock());

        enforceType(PropertyType::boolean);

        return container.boolean;
    }

    Property::BooleanType Property::booleanValue() const
    {
        assert(parent.haveLock());

        enforceType(PropertyType::boolean);
        enforceDefined();

        return container.boolean;
    }

    void Property::booleanValue(const BooleanType value)
    {
        assert(parent.haveLock());

        enforcePublicMutable();

        booleanRef() = value;
        hasValue = true;
    }

    Property::FileType& Property::fileRef()
    {
        assert(parent.haveLock());

        enforceType(PropertyType::file);

        return *container.file;
    }

    Property::FileType Property::fileValue() const
    {
        assert(parent.haveLock());

        enforceType(PropertyType::file);
        enforceDefined();

        return *container.file;
    }

    void Property::fileValue(const FileType& value)
    {
        assert(parent.haveLock());

        enforcePublicMutable();

        fileRef() = value;
        hasValue = true;
    }

    Property::IntegerType& Property::integerRef()
    {
        assert(parent.haveLock());

        enforceType(PropertyType::integer);

        return container.integer;
    }

    Property::IntegerType Property::integerValue() const
    {
        assert(parent.haveLock());

        enforceType(PropertyType::integer);
        enforceDefined();

        return container.integer;
    }

    void Property::integerValue(const IntegerType value)
    {
        assert(parent.haveLock());

        enforcePublicMutable();

        integerRef() = value;
        hasValue = true;
    }

    Property::RealType& Property::realRef()
    {
        assert(parent.haveLock());

        enforceType(PropertyType::real);

        return container.real;
    }

    Property::RealType Property::realValue() const
    {
        assert(parent.haveLock());

        enforceType(PropertyType::real);
        enforceDefined();

        return container.real;
    }

    void Property::realValue(const RealType value)
    {
        assert(parent.haveLock());

        enforcePublicMutable();

        realRef() = value;
        hasValue = true;
    }

    Property::SizeType& Property::sizeRef()
    {
        assert(parent.haveLock());

        enforceType(PropertyType::size);

        return container.size;
    }

    Property::SizeType Property::sizeValue() const
    {
        assert(parent.haveLock());

        enforceType(PropertyType::size);
        enforceDefined();

        return container.size;
    }

    void Property::sizeValue(const SizeType value)
    {
        assert(parent.haveLock());

        enforcePublicMutable();

        sizeRef() = value;
        hasValue = true;
    }

    Property::StringType& Property::stringRef()
    {
        assert(parent.haveLock());

        enforceType(PropertyType::string);

        return *container.string;
    }

    Property::StringType Property::stringValue() const
    {
        assert(parent.haveLock());

        enforceType(PropertyType::string);
        enforceDefined();

        return *container.string;
    }

    void Property::stringValue(const StringType& value)
    {
        assert(parent.haveLock());

        enforcePublicMutable();

        stringRef() = value;
        hasValue = true;
    }

    std::any Property::anyValue() const
    {
        if (! hasValue) return std::any(nullptr);

        switch (type)
        {
            case PropertyType::boolean: return std::any(booleanValue());
            case PropertyType::file: return std::any(fileValue());
            case PropertyType::integer: return std::any(integerValue());
            case PropertyType::real: return std::any(realValue());
            case PropertyType::size: return std::any(sizeValue());
            case PropertyType::string: return std::any(stringValue());
        }
    }

    void Property::anyValue(const std::any& value)
    {
        assert(parent.haveLock());

        enforcePublicMutable();

        set(value);
    }

    std::string asString(const PropertyType type) noexcept
    {
        switch (type)
        {
            case PropertyType::boolean: return "boolean";
            case PropertyType::file: return "file";
            case PropertyType::integer: return "integer";
            case PropertyType::real: return "real";
            case PropertyType::size: return "size";
            case PropertyType::string: return "string";
        }
    }

    std::ostream& operator<<(std::ostream& os, const PropertyType& type) noexcept
    {
        os << asString(type);
        return os;
    }
}
