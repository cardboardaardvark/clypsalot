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
#include <string>

#include <clypsalot/forward.hxx>

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

    class Property
    {
        friend Object;

        public:
        using BooleanType = bool;
        using FileType = std::filesystem::path;
        using IntegerType = int;
        using RealType = float;
        using SizeType = size_t;
        using StringType = std::string;
        using Flags = uint_fast8_t;

        enum FlagValues : Flags
        {
            NoFlags = 0,
            Configurable = 1 << 0,
            Required = 1 << 1,
            PublicMutable = 1 << 2,
        };

        private:
        union Container
        {
            BooleanType boolean;
            FileType* file;
            IntegerType integer;
            RealType real;
            SizeType size;
            StringType* string;
        };

        const Lockable& m_parent;
        Container m_container;
        const std::string m_name;
        const PropertyType m_type;
        const Flags m_flags;
        bool m_hasValue = false;

        protected:
        void set(const std::any& in_value);
        void defined(const bool in_defined);
        void enforcePublicMutable() const;
        void enforceType(const PropertyType in_enforceType) const;
        void enforceDefined() const;
        BooleanType& booleanRef();
        FileType& fileRef();
        IntegerType& integerRef();
        RealType& realRef();
        SizeType& sizeRef();
        StringType& stringRef();

        public:
        Property(const Lockable& in_parent, const PropertyConfig& in_config);
        Property(const Property&) = delete;
        ~Property();
        void operator=(const Property&) = delete;
        const std::string& name() const noexcept;
        PropertyType type() const noexcept;
        Flags flags() const noexcept;
        bool hasFlag(const Flags in_flags) const noexcept;
        bool defined() const noexcept;
        std::string valueToString() const;
        BooleanType booleanValue() const;
        void booleanValue(const BooleanType in_value);
        FileType fileValue() const;
        void fileValue(const FileType& in_value);
        IntegerType integerValue() const;
        void integerValue(const IntegerType in_value);
        RealType realValue() const;
        void realValue(const RealType in_value);
        SizeType sizeValue() const;
        void sizeValue(const SizeType in_value);
        StringType stringValue() const;
        void stringValue(const StringType& in_value);
        std::any anyValue() const;
        void anyValue(const std::any& in_value);
    };

    struct PropertyConfig
    {
        const std::string name;
        const PropertyType type;
        const Property::Flags flags;
        const std::any initial;
    };

    std::string toString(const PropertyType in_type) noexcept;
    std::ostream& operator<<(std::ostream& in_os, const PropertyType& in_rhs) noexcept;
}
