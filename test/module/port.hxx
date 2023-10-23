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

#include <clypsalot/port.hxx>

namespace Clypsalot
{
    // Manual test port
    struct MTestPortType : public PortType
    {
        static const std::string typeName;
        static const MTestPortType singleton;

        MTestPortType();
        virtual PortLink* makeLink(OutputPort& from, InputPort& to) const override;
    };

    class MTestOutputPort : public OutputPort
    {
        bool readyFlag = false;

        public:
        MTestOutputPort(const std::string& name, Object& parent);
        virtual bool ready() const noexcept override;
        void setReady(const bool ready) noexcept;
    };

    class MTestInputPort : public InputPort
    {
        bool readyFlag = false;

        public:
        MTestInputPort(const std::string& name, Object& parent);
        virtual bool ready() const noexcept override;
        void setReady(const bool ready) noexcept;
    };

    class MTestPortLink : public PortLink
    {
        public:
        MTestPortLink(MTestOutputPort& from, MTestInputPort& to);
    };

    // Processing test port
    struct PTestPortType : public PortType
    {
        static const std::string typeName;
        static const PTestPortType singleton;

        PTestPortType();
        virtual PortLink* makeLink(OutputPort& from, InputPort& to) const override;
    };

    class PTestOutputPort : public OutputPort
    {
        public:
        PTestOutputPort(const std::string& name, Object& parent);
        virtual bool ready() const noexcept override;
    };

    class PTestInputPort : public InputPort
    {
        public:
        PTestInputPort(const std::string& name, Object& parent);
        virtual bool ready() const noexcept override;
    };

    class PTestPortLink : public PortLink
    {
        bool dirtyFlag = false;

        public:
        PTestPortLink(PTestOutputPort& from, PTestInputPort& to);
        bool dirty() const noexcept;
        void dirty(const bool isDirty) noexcept;
    };
}
