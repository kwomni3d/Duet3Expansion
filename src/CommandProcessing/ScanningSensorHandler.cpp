/*
 * ScanningSensorHandler.cpp
 *
 *  Created on: 16 Jun 2023
 *      Author: David
 */

#include "ScanningSensorHandler.h"

#if SUPPORT_LDC1612

#include <Hardware/LDC1612.h>
#include <Platform/Platform.h>
#include <CanMessageFormats.h>
#include <AnalogIn.h>

static LDC1612 *sensor = nullptr;
static AnalogIn::AdcTaskHookFunction *oldHookFunction = nullptr;
static uint32_t lastReading = 0;
static volatile bool isCalibrating = false;

static void LDC1612TaskHook() noexcept
{
	// The LDC1612 generates lots of bus errors if we try to read the data when no new data is available
	if (!isCalibrating && sensor->IsChannelReady(0))
	{
		uint32_t val;
		if (sensor->GetChannelResult(0, val))		// if no error
		{
			lastReading = val;						// save all 28 bits of data + 4 error bits
		}
		else
		{
			lastReading = 0;
		}
	}

	if (oldHookFunction != nullptr)
	{
		oldHookFunction();
	}
}

void ScanningSensorHandler::Init() noexcept
{
	// Set up the external clock to the LDC1612.
	// The higher the better, but the maximum is 40MHz
#if defined(SAMMYC21)
	// Assume we are using a LDC1612 breakout board with its own crystal, so we don't need to generate a clock
#elif defined(TOOL1LC) || defined(SZP)
	// We use the 96MHz DPLL output divided by 3 to get 32MHz. It might be better to use 25MHz from the crystal directly for better stability.
	ConfigureGclk(GclkNumPA23, GclkSource::dpll, 3, true);
	SetPinFunction(LDC1612ClockGenPin, GpioPinFunction::H);
#elif defined(TOOL1RR)
	// We use the 120MHz DPLL output divided by 4 to get 30MHz. It might be better to use 25MHz from the crystal directly for better stability.
	ConfigureGclk(GclkNumPB11, GclkSource::dpll0, 4, true);
	SetPinFunction(LDC1612ClockGenPin, GpioPinFunction::M);
#else
# error LDC support not implemented for this processor
#endif

	sensor = new LDC1612(Platform::GetSharedI2C());

	if (sensor->CheckPresent())
	{
		sensor->SetDefaultConfiguration(0);
		oldHookFunction = AnalogIn::SetTaskHook(LDC1612TaskHook);
	}
	else
	{
		DeleteObject(sensor);
#if defined(TOOL1LC) || defined(SZP)
		ClearPinFunction(LDC1612ClockGenPin);
#endif
	}
}

bool ScanningSensorHandler::IsPresent() noexcept
{
	return sensor != nullptr;
}

uint32_t ScanningSensorHandler::GetReading() noexcept
{
	return lastReading;
}

GCodeResult ScanningSensorHandler::SetOrCalibrateCurrent(uint32_t param, const StringRef& reply, uint8_t& extra) noexcept
{
	if (sensor != nullptr)
	{
		if (param == CanMessageChangeInputMonitorNew::paramAutoCalibrateDriveLevelAndReport)
		{
			isCalibrating = true;
			const bool ok = (sensor->CalibrateDriveCurrent(0));
			isCalibrating = false;
			if (ok)
			{
				extra = sensor->GetDriveCurrent(0);
				reply.printf("Calibration successful, sensor drive current is %u", extra);
				return GCodeResult::ok;
			}
			reply.copy("failed to calibrate sensor drive current");
		}
		else if (param == CanMessageChangeInputMonitorNew::paramReportDriveLevel)
		{
			extra = sensor->GetDriveCurrent(0);
			reply.printf("Sensor drive current is %u", extra);
			return GCodeResult::ok;
		}
		else
		{
			if (param > 31)
			{
				param = 31;
			}
			isCalibrating = true;
			const bool ok = (sensor->SetDriveCurrent(0, param));
			isCalibrating = false;
			if (ok)
			{
				extra = param;
				return GCodeResult::ok;
			}
			reply.copy("failed to set sensor drive current");
		}
	}
	extra = 0xFF;
	return GCodeResult::error;
}

void ScanningSensorHandler::AppendDiagnostics(const StringRef& reply) noexcept
{
	reply.lcat("Inductive sensor: ");
	if (IsPresent())
	{
		sensor->AppendDiagnostics(reply);
	}
	else
	{
		reply.cat("not found");
	}
}

#endif

// End
