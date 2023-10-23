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

#pragma once

#include <any>
#include <cstdint>
#include <filesystem>
#include <initializer_list>
#include <string>

#include <clypsalot/forward.hxx>
#include <clypsalot/thread.hxx>

namespace Clypsalot
{
    enum class PropertyType : uint_fast8_t
    {
        boolean,
        file,
        integer,
        real,
        size,
        string
    };

    struct PropertyConfig
    {
        const std::string name;
        const PropertyType type;
        const bool publicMutable;
        const std::any initial;
    };

    using PropertyList = std::initializer_list<PropertyConfig>;

    class Property
    {
        friend Object;

        using BooleanType = bool;
        using FileType = std::filesystem::path;
        using IntegerType = int;
        using RealType = float;
        using SizeType = size_t;
        using StringType = std::string;

        union Container
        {
            BooleanType boolean;
            FileType* file;
            IntegerType integer;
            RealType real;
            SizeType size;
            StringType* string;
        };

        const Lockable& parent;
        Container container;
        bool hasValue = false;

        protected:
        void set(const std::any& value);
        bool defined(const bool defined);
        BooleanType& booleanRef();
        FileType& fileRef();
        IntegerType& integerRef();
        RealType& realRef();
        SizeType& sizeRef();
        StringType& stringRef();

        public:
        const std::string name;
        const PropertyType type;
        const bool publicMutable;

        Property(const Lockable& parent, const std::string& name, const PropertyType type, const bool publicMutable, const std::any& initial = nullptr);
        Property(const Property&) = delete;
        ~Property();
        void operator=(const Property&) = delete;
        bool defined();
        BooleanType booleanValue();
        void booleanValue(const BooleanType value);
        FileType fileValue();
        void fileValue(const FileType& value);
        IntegerType integerValue();
        void integerValue(const IntegerType value);
        RealType realValue();
        void realValue(const RealType value);
        SizeType sizeValue();
        void sizeValue(const SizeType value);
        StringType stringValue();
        void stringValue(const StringType& value);
    };
}
