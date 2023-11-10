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

    struct PropertyConfig
    {
        const std::string name;
        const PropertyType type;
        const bool configurable;
        const bool required;
        const bool publicMutable;
        const std::any initial;
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
        bool m_hasValue = false;

        protected:
        void set(const std::any& value);
        bool defined(const bool defined);
        void enforcePublicMutable() const;
        void enforceType(const PropertyType enforceType) const;
        void enforceDefined() const;
        BooleanType& booleanRef();
        FileType& fileRef();
        IntegerType& integerRef();
        RealType& realRef();
        SizeType& sizeRef();
        StringType& stringRef();

        public:
        const std::string m_name;
        const PropertyType m_type;
        const bool m_configurable;
        const bool m_required;
        const bool m_publicMutable;

        Property(const Lockable& parent, const std::string& name, const PropertyType type, const bool configurable, const bool required, const bool publicMutable, const std::any& initial = nullptr);
        Property(const Property&) = delete;
        ~Property();
        void operator=(const Property&) = delete;
        bool defined() const;
        std::string valueToString() const;
        BooleanType booleanValue() const;
        void booleanValue(const BooleanType value);
        FileType fileValue() const;
        void fileValue(const FileType& value);
        IntegerType integerValue() const;
        void integerValue(const IntegerType value);
        RealType realValue() const;
        void realValue(const RealType value);
        SizeType sizeValue() const;
        void sizeValue(const SizeType value);
        StringType stringValue() const;
        void stringValue(const StringType& value);
        std::any anyValue() const;
        void anyValue(const std::any& value);
    };

    std::string toString(const PropertyType type) noexcept;
    std::ostream& operator<<(std::ostream& os, const PropertyType& type) noexcept;
}
