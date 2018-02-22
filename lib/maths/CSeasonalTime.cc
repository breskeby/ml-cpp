/*
 * Copyright Elasticsearch B.V. and/or licensed to Elasticsearch B.V. under one
 * or more contributor license agreements. Licensed under the Elastic License;
 * you may not use this file except in compliance with the Elastic License.
 */

#include <maths/CSeasonalTime.h>

#include <core/CLogger.h>
#include <core/Constants.h>
#include <core/CPersistUtils.h>
#include <core/CStatePersistInserter.h>
#include <core/CStateRestoreTraverser.h>

#include <maths/CChecksum.h>
#include <maths/CIntegerTools.h>

#include <boost/array.hpp>
#include <boost/numeric/conversion/bounds.hpp>

#include <cstddef>
#include <string>

namespace ml
{
namespace maths
{
namespace
{
// DO NOT change the existing tags if new sub-classes are added.
const std::string DIURNAL_TIME_TAG("a");
const std::string ARBITRARY_PERIOD_TIME_TAG("b");
}

//////// CSeasonalTime ////////

CSeasonalTime::CSeasonalTime(void) :
        m_Period(0), m_RegressionOrigin(0)
{}

CSeasonalTime::CSeasonalTime(core_t::TTime period) :
        m_Period(period), m_RegressionOrigin(0)
{}

CSeasonalTime::~CSeasonalTime(void) {}

double CSeasonalTime::periodic(core_t::TTime time) const
{
    return static_cast<double>((time - this->startOfWindow(time)) % m_Period);
}

double CSeasonalTime::regression(core_t::TTime time) const
{
    return  static_cast<double>(time - m_RegressionOrigin)
          / static_cast<double>(this->regressionTimeScale());
}

double CSeasonalTime::regressionInterval(core_t::TTime start, core_t::TTime end) const
{
    return  static_cast<double>(end - start)
          / static_cast<double>(this->regressionTimeScale());
}

core_t::TTime CSeasonalTime::startOfWindowRepeat(core_t::TTime time) const
{
    return this->startOfWindowRepeat(this->windowRepeatStart(), time);
}

core_t::TTime CSeasonalTime::startOfWindow(core_t::TTime time) const
{
    return this->startOfWindowRepeat(this->windowRepeatStart() + this->windowStart(), time);
}

bool CSeasonalTime::inWindow(core_t::TTime time) const
{
    time = time - this->startOfWindowRepeat(time);
    return time >= this->windowStart() && time < this->windowEnd();
}

core_t::TTime CSeasonalTime::period(void) const
{
    return m_Period;
}

void CSeasonalTime::period(core_t::TTime period)
{
    m_Period = period;
}

core_t::TTime CSeasonalTime::regressionOrigin(void) const
{
    return m_RegressionOrigin;
}

void CSeasonalTime::regressionOrigin(core_t::TTime origin)
{
    m_RegressionOrigin = origin;
}

CSeasonalTime::TTimeTimePr CSeasonalTime::window(void) const
{
    return {this->windowStart(), this->windowEnd()};
}

core_t::TTime CSeasonalTime::windowLength(void) const
{
    return this->windowEnd() - this->windowStart();
}

bool CSeasonalTime::windowed(void) const
{
    return this->windowLength() < this->windowRepeat();
}

double CSeasonalTime::fractionInWindow(void) const
{
    return  static_cast<double>(std::max(this->period(), this->windowLength()))
          / static_cast<double>(this->windowRepeat());
}

double CSeasonalTime::scaleDecayRate(double decayRate,
                                     core_t::TTime fromPeriod,
                                     core_t::TTime toPeriod)
{
    return static_cast<double>(fromPeriod) / static_cast<double>(toPeriod) * decayRate;
}

core_t::TTime CSeasonalTime::startOfWindowRepeat(core_t::TTime offset, core_t::TTime time) const
{
    return offset + CIntegerTools::floor(time - offset, this->windowRepeat());
}

//////// CDiurnalTime ////////

CDiurnalTime::CDiurnalTime(void) :
        m_StartOfWeek(0), m_WindowStart(0), m_WindowEnd(0)
{}

CDiurnalTime::CDiurnalTime(core_t::TTime startOfWeek,
                           core_t::TTime windowStart,
                           core_t::TTime windowEnd,
                           core_t::TTime period) :
        CSeasonalTime(period),
        m_StartOfWeek(startOfWeek),
        m_WindowStart(windowStart),
        m_WindowEnd(windowEnd)
{}

CDiurnalTime *CDiurnalTime::clone(void) const
{
    return new CDiurnalTime(*this);
}

bool CDiurnalTime::fromString(const std::string &value)
{
    boost::array<core_t::TTime, 5> times;
    if (core::CPersistUtils::fromString(value, times))
    {
        m_StartOfWeek = times[0];
        m_WindowStart = times[1];
        m_WindowEnd   = times[2];
        this->period(times[3]);
        this->regressionOrigin(times[4]);
        return true;
    }
    return false;
}

std::string CDiurnalTime::toString(void) const
{
    boost::array<core_t::TTime, 5> times;
    times[0] = m_StartOfWeek;
    times[1] = m_WindowStart;
    times[2] = m_WindowEnd;
    times[3] = this->period();
    times[4] = this->regressionOrigin();
    return core::CPersistUtils::toString(times);
}

core_t::TTime CDiurnalTime::windowRepeat(void) const
{
    return core::constants::WEEK;
}

core_t::TTime CDiurnalTime::windowRepeatStart(void) const
{
    return m_StartOfWeek;
}

core_t::TTime CDiurnalTime::windowStart(void) const
{
    return m_WindowStart;
}

core_t::TTime CDiurnalTime::windowEnd(void) const
{
    return m_WindowEnd;
}

uint64_t CDiurnalTime::checksum(uint64_t seed) const
{
    seed = CChecksum::calculate(seed, m_StartOfWeek);
    seed = CChecksum::calculate(seed, m_WindowStart);
    seed = CChecksum::calculate(seed, m_WindowEnd);
    return CChecksum::calculate(seed, this->period());
}

core_t::TTime CDiurnalTime::regressionTimeScale(void) const
{
    return core::constants::WEEK;
}

//////// CGeneralPeriodTime ////////

CGeneralPeriodTime::CGeneralPeriodTime(void) {}

CGeneralPeriodTime::CGeneralPeriodTime(core_t::TTime period) :
        CSeasonalTime(period)
{}

CGeneralPeriodTime *CGeneralPeriodTime::clone(void) const
{
    return new CGeneralPeriodTime(*this);
}

bool CGeneralPeriodTime::fromString(const std::string &value)
{
    boost::array<core_t::TTime, 2> times;
    if (core::CPersistUtils::fromString(value, times))
    {
        this->period(times[0]);
        this->regressionOrigin(times[1]);
        return true;
    }
    return false;
}

std::string CGeneralPeriodTime::toString(void) const
{
    boost::array<core_t::TTime, 2> times;
    times[0] = this->period();
    times[1] = this->regressionOrigin();
    return core::CPersistUtils::toString(times);
}

core_t::TTime CGeneralPeriodTime::windowRepeat(void) const
{
    return this->period();
}

core_t::TTime CGeneralPeriodTime::windowRepeatStart(void) const
{
    return 0;
}

core_t::TTime CGeneralPeriodTime::windowStart(void) const
{
    return 0;
}

core_t::TTime CGeneralPeriodTime::windowEnd(void) const
{
    return this->period();
}

uint64_t CGeneralPeriodTime::checksum(uint64_t seed) const
{
    return CChecksum::calculate(seed, this->period());
}

core_t::TTime CGeneralPeriodTime::regressionTimeScale(void) const
{
    return std::max(core::constants::WEEK, this->period());
}

//////// CSeasonalTimeStateSerializer ////////

bool CSeasonalTimeStateSerializer::acceptRestoreTraverser(TSeasonalTimePtr &result,
                                                          core::CStateRestoreTraverser &traverser)
{
    std::size_t numResults = 0;

    do
    {
        const std::string &name = traverser.name();
        if (name == DIURNAL_TIME_TAG)
        {
            result.reset(new CDiurnalTime);
            result->fromString(traverser.value());
            ++numResults;
        }
        else if (name == ARBITRARY_PERIOD_TIME_TAG)
        {
            result.reset(new CGeneralPeriodTime);
            result->fromString(traverser.value());
            ++numResults;
        }
        else
        {
            LOG_ERROR("No seasonal time corresponds to name " << traverser.name());
            return false;
        }
    }
    while (traverser.next());

    if (numResults != 1)
    {
        LOG_ERROR("Expected 1 (got " << numResults << ") seasonal time tags");
        result.reset();
        return false;
    }

    return true;
}

void CSeasonalTimeStateSerializer::acceptPersistInserter(const CSeasonalTime &time,
                                                         core::CStatePersistInserter &inserter)
{
    if (dynamic_cast<const CDiurnalTime*>(&time) != 0)
    {
        inserter.insertValue(DIURNAL_TIME_TAG, time.toString());
    }
    else if (dynamic_cast<const CGeneralPeriodTime*>(&time) != 0)
    {
        inserter.insertValue(ARBITRARY_PERIOD_TIME_TAG, time.toString());
    }
    else
    {
        LOG_ERROR("Seasonal time with type " << typeid(time).name() << " has no defined name");
    }
}

}
}