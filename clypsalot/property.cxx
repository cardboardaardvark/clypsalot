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
#include <clypsalot/util.hxx>

#define ENFORCE_TYPE(enforceType)\
    if (type != PropertyType::enforceType)\
    {\
        throw TypeError(makeString("Property ", name, " is not of ", #enforceType, " type"));\
    }

#define ENFORCE_PUBLIC_MUTABLE()\
    if (! publicMutable) {\
        throw ImmutableError(makeString("Property ", name, " is not mutable"));\
    }

#define ENFORCE_DEFINED()\
    if (! hasValue) {\
        throw UndefinedError(makeString("Property ", name, " does not have a value"));\
    }\

namespace Clypsalot
{
    Property::Property(const Lockable& parent, const std::string& name, const PropertyType type, const bool publicMutable, const std::any& initial) :
        parent(parent),
        name(name),
        type(type),
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

    bool Property::defined()
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

    Property::BooleanType& Property::booleanRef()
    {
        assert(parent.haveLock());

        ENFORCE_TYPE(boolean);

        return container.boolean;
    }

    Property::BooleanType Property::booleanValue()
    {
        assert(parent.haveLock());

        ENFORCE_DEFINED();

        return booleanRef();
    }

    void Property::booleanValue(const BooleanType value)
    {
        assert(parent.haveLock());

        ENFORCE_PUBLIC_MUTABLE();

        booleanRef() = value;
        hasValue = true;
    }

    Property::FileType& Property::fileRef()
    {
        assert(parent.haveLock());

        ENFORCE_TYPE(file);

        return *container.file;
    }

    Property::FileType Property::fileValue()
    {
        assert(parent.haveLock());

        ENFORCE_DEFINED();

        return fileRef();
    }

    void Property::fileValue(const FileType& value)
    {
        assert(parent.haveLock());

        ENFORCE_PUBLIC_MUTABLE();

        fileRef() = value;
        hasValue = true;
    }

    Property::IntegerType& Property::integerRef()
    {
        assert(parent.haveLock());

        ENFORCE_TYPE(integer);

        return container.integer;
    }

    Property::IntegerType Property::integerValue()
    {
        assert(parent.haveLock());

        ENFORCE_DEFINED();

        return integerRef();
    }

    void Property::integerValue(const IntegerType value)
    {
        assert(parent.haveLock());

        ENFORCE_PUBLIC_MUTABLE();

        integerRef() = value;
        hasValue = true;
    }

    Property::RealType& Property::realRef()
    {
        assert(parent.haveLock());

        ENFORCE_TYPE(real);

        return container.real;
    }

    Property::RealType Property::realValue()
    {
        assert(parent.haveLock());

        ENFORCE_DEFINED();

        return realRef();
    }

    void Property::realValue(const RealType value)
    {
        assert(parent.haveLock());

        ENFORCE_PUBLIC_MUTABLE();

        realRef() = value;
        hasValue = true;
    }

    Property::SizeType& Property::sizeRef()
    {
        assert(parent.haveLock());

        ENFORCE_TYPE(size);

        return container.size;
    }

    Property::SizeType Property::sizeValue()
    {
        assert(parent.haveLock());

        ENFORCE_DEFINED();

        return sizeRef();
    }

    void Property::sizeValue(const SizeType value)
    {
        assert(parent.haveLock());

        ENFORCE_PUBLIC_MUTABLE();

        sizeRef() = value;
        hasValue = true;
    }

    Property::StringType& Property::stringRef()
    {
        assert(parent.haveLock());

        ENFORCE_TYPE(string);

        return *container.string;
    }

    Property::StringType Property::stringValue()
    {
        assert(parent.haveLock());

        ENFORCE_DEFINED();

        return stringRef();
    }

    void Property::stringValue(const StringType& value)
    {
        assert(parent.haveLock());

        ENFORCE_PUBLIC_MUTABLE();

        stringRef() = value;
        hasValue = true;
    }
}
