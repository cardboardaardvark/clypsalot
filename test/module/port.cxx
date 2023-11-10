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

#include <clypsalot/catalog.hxx>
#include <clypsalot/error.hxx>
#include <clypsalot/logger.hxx>

#include "test/module/port.hxx"

namespace Clypsalot
{
    const MTestPortType MTestPortType::singleton = MTestPortType();
    const std::string MTestPortType::typeName = "mtest";
    const PTestPortType PTestPortType::singleton = PTestPortType();
    const std::string PTestPortType::typeName = "ptest";

    MTestPortType::MTestPortType() :
        PortType(MTestPortType::typeName)
    { }

    PortLink* MTestPortType::makeLink(OutputPort& from, InputPort& to) const
    {
        const auto& ourTypeInfo = typeid(MTestPortType);
        const auto& fromType = from.type();
        const auto& toType = to.type();

        if (typeid(fromType) != ourTypeInfo || typeid(toType) != ourTypeInfo)
        {
            throw TypeError("Incompatible port types when creating a link");
        }

        return new MTestPortLink(
            dynamic_cast<MTestOutputPort&>(from),
            dynamic_cast<MTestInputPort&>(to)
        );
    }

    MTestOutputPort::MTestOutputPort(const std::string& name, Object& parent) :
        OutputPort(name, MTestPortType::singleton, parent)
    { }

    bool MTestOutputPort::ready() const noexcept
    {
        assert(m_parent.haveLock());
        return m_readyFlag;
    }

    void MTestOutputPort::setReady(const bool ready) noexcept
    {
        assert(m_parent.haveLock());
        m_readyFlag = ready;
        PORT_LOGGER(trace, "ready=", m_readyFlag);
    }

    MTestInputPort::MTestInputPort(const std::string& name, Object& parent) :
        InputPort(name, MTestPortType::singleton, parent)
    { }

    bool MTestInputPort::ready() const noexcept
    {
        assert(m_parent.haveLock());
        return m_readyFlag;
    }

    void MTestInputPort::setReady(const bool ready) noexcept
    {
        assert(m_parent.haveLock());
        m_readyFlag = ready;
        PORT_LOGGER(trace, "ready=", m_readyFlag);
    }

    MTestPortLink::MTestPortLink(MTestOutputPort& from, MTestInputPort& to) :
        PortLink(from, to)
    { }

    PTestPortType::PTestPortType() :
        PortType(PTestPortType::typeName)
    { }

    PortLink* PTestPortType::makeLink(OutputPort& from, InputPort& to) const
    {
        const auto& ourTypeInfo = typeid(PTestPortType);
        const auto& fromType = from.type();
        const auto& toType = to.type();

        if (typeid(fromType) != ourTypeInfo || typeid(toType) != ourTypeInfo)
        {
            throw TypeError("Incompatible port types when creating a link");
        }

        return new PTestPortLink(
            dynamic_cast<PTestOutputPort&>(from),
            dynamic_cast<PTestInputPort&>(to)
        );
    }

    PTestOutputPort::PTestOutputPort(const std::string& name, Object& parent) :
        OutputPort(name, PTestPortType::singleton, parent)
    { }

    bool PTestOutputPort::ready() const noexcept
    {
        assert(m_parent.haveLock());

        if (portLinks.size() == 0)
        {
            return false;
        }

        for (const auto baseLink : portLinks)
        {
            auto link = dynamic_cast<PTestPortLink*>(baseLink);

            if (link->dirty())
            {
                return false;
            }
        }

        return true;
    }

    PTestInputPort::PTestInputPort(const std::string& name, Object& parent) :
        InputPort(name, PTestPortType::singleton, parent)
    { }

    bool PTestInputPort::ready() const noexcept
    {
        assert(m_parent.haveLock());

        if (portLinks.size() == 0)
        {
            return false;
        }

        for (const auto baseLink : portLinks)
        {
            auto link = dynamic_cast<PTestPortLink*>(baseLink);

            if (! link->dirty())
            {
                return false;
            }
        }

        return true;
    }

    PTestPortLink::PTestPortLink(PTestOutputPort& from, PTestInputPort& to) :
        PortLink(from, to)
    { }

    bool PTestPortLink::dirty() const noexcept
    {
        std::scoped_lock lock(m_mutex);

        return m_dirtyFlag;
    }

    void PTestPortLink::dirty(const bool isDirty) noexcept
    {
        std::scoped_lock lock(m_mutex);

        m_dirtyFlag = isDirty;
    }
}
