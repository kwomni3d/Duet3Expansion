/*
 * MoveTimings.h
 *
 *  Created on: 28 Nov 2023
 *      Author: David
 */

#ifndef SRC_MOVEMENT_MOVETIMING_H_
#define SRC_MOVEMENT_MOVETIMING_H_

#include <RepRapFirmware.h>
#include "StepTimer.h"

namespace MoveTiming
{
	// Note on the following constant:
	// If we calculate the step interval on every clock, we reach a point where the calculation time exceeds the step interval.
	// The worst case is pure Z movement on a delta. On a Mini Kossel with 80 steps/mm with this firmware running on a Duet (84MHx SAM3X8 processor),
	// the calculation can just be managed in time at speeds of 15000mm/min (step interval 50us), but not at 20000mm/min (step interval 37.5us).
	// Therefore, where the step interval falls below 60us, we don't calculate on every step.
	// Note: the above measurements were taken some time ago, before some firmware optimisations.
#if SAME70
	// Use the same defaults as for the SAM4E for now.
	constexpr uint32_t MinCalcInterval = (40 * StepTimer::StepClockRate)/1000000;				// the smallest sensible interval between calculations (40us) in step timer clocks
	constexpr uint32_t HiccupTime = (30 * StepTimer::StepClockRate)/1000000;					// how long we hiccup for in step timer clocks
#elif SAM4E || SAM4S || SAME5x
	constexpr uint32_t MinCalcInterval = (40 * StepTimer::StepClockRate)/1000000;				// the smallest sensible interval between calculations (40us) in step timer clocks
	constexpr uint32_t HiccupTime = (30 * StepTimer::StepClockRate)/1000000;					// how long we hiccup for in step timer clocks
#elif SAMC21 || RP2040
	constexpr uint32_t MinCalcInterval = (100 * StepTimer::StepClockRate)/1000000;				// the smallest sensible interval between calculations (40us) in step timer clocks
	constexpr uint32_t HiccupTime = (50 * StepTimer::StepClockRate)/1000000;					// how long we hiccup for
#else
# error Unsupported processor
#endif
	constexpr uint32_t MinInterruptInterval = (6 * StepTimer::StepClockRate)/1000000;			// Minimum interval between step timer interrupts, in step clocks; about 6us. See StepTimer::ScheduleTimerInterrupt.
	constexpr uint32_t MaxStepInterruptTime = 10 * MinInterruptInterval;						// the maximum time we spend looping in the ISR, in step clocks
	constexpr uint32_t HiccupIncrement = HiccupTime/2;											// how much we increase the hiccup time by on each attempt

	constexpr uint32_t UsualMinimumPreparedTime = StepTimer::StepClockRate/10;					// 100ms
	constexpr uint32_t AbsoluteMinimumPreparedTime = StepTimer::StepClockRate/20;				// 50ms
	constexpr uint32_t MaximumMoveStartAdvanceClocks = StepTimer::StepClockRate/1000;			// 1ms

	constexpr uint32_t StandardMoveWakeupInterval = 500;										// milliseconds
	constexpr uint32_t MachineCoordinateUpdateInterval = 200;									// milliseconds
}

#endif /* SRC_MOVEMENT_MOVETIMING_H_ */