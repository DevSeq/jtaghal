/***********************************************************************************************************************
*                                                                                                                      *
* ANTIKERNEL v0.1                                                                                                      *
*                                                                                                                      *
* Copyright (c) 2012-2018 Andrew D. Zonenberg                                                                          *
* All rights reserved.                                                                                                 *
*                                                                                                                      *
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that the     *
* following conditions are met:                                                                                        *
*                                                                                                                      *
*    * Redistributions of source code must retain the above copyright notice, this list of conditions, and the         *
*      following disclaimer.                                                                                           *
*                                                                                                                      *
*    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the       *
*      following disclaimer in the documentation and/or other materials provided with the distribution.                *
*                                                                                                                      *
*    * Neither the name of the author nor the names of any contributors may be used to endorse or promote products     *
*      derived from this software without specific prior written permission.                                           *
*                                                                                                                      *
* THIS SOFTWARE IS PROVIDED BY THE AUTHORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   *
* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL *
* THE AUTHORS BE HELD LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES        *
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR       *
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE       *
* POSSIBILITY OF SUCH DAMAGE.                                                                                          *
*                                                                                                                      *
***********************************************************************************************************************/

/**
	@file
	@author Andrew D. Zonenberg
	@brief Implementation of ARMCortexA9
 */

#include "jtaghal.h"
#include "DebuggableDevice.h"
#include "ARMAPBDevice.h"
#include "ARMDebugPort.h"
#include "ARMDebugAccessPort.h"
#include "ARMDebugMemAccessPort.h"
#include "ARMCortexA9.h"

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

ARMCortexA9::ARMCortexA9(ARMDebugMemAccessPort* ap, uint32_t address, ARMDebugPeripheralIDRegisterBits idreg)
	: ARMAPBDevice(ap, address, idreg)
{
	LogTrace("Found ARM Cortex-A9 at %08x, probing...\n", address);
	LogIndenter li;

	//Read the Debug ID register and extract flags
	m_deviceID.word = ReadRegisterByIndex(DBGDIDR);
	m_breakpoints = m_deviceID.bits.bpoints_minus_one + 1;
	m_context_breakpoints = m_deviceID.bits.context_bpoints_minus_one + 1;
	m_watchpoints = m_deviceID.bits.wpoints_minus_one + 1;
	m_hasDevid = m_deviceID.bits.has_dbgdevid;
	m_hasSecExt = m_deviceID.bits.sec_ext;
	m_hasSecureHalt = m_deviceID.bits.sec_ext && !m_deviceID.bits.no_secure_halt;
	m_revision = m_deviceID.bits.revision;
	m_variant = m_deviceID.bits.variant;
	if(m_deviceID.bits.pcsr_legacy_addr)
		m_pcsrIndex = DBGPCSR_LEGACY;
	else
		m_pcsrIndex = DBGPCSR;

	//DBGDSMCR turn off MMU

	//TODO: Write to DBGDRCR to halt the CPU (C11.11.17)

	//TODO: Write to DBGDSCCR to force write-through cache (C11.11.19)

	//DBGDEVID[3:0] 2226
}

ARMCortexA9::~ARMCortexA9()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// General device info

void ARMCortexA9::PrintInfo()
{
	LogVerbose("%s\n", GetDescription().c_str());
	LogIndenter li;

	PrintIDRegister(m_deviceID);

	//Read DBGDSCR to get status stuff (TODO: make struct) for this
	//uint32_t dbgdscr = ReadRegisterByIndex(DBGDSCR_EXT);
	//LogDebug("DBGDSCR = %x\n", dbgdscr);

	//Pins of interest are MIO bank 1, pins 50/51

	//Read MCTRL

	//Read PSS_IDCODE from the zynq
	//uint32_t pss_idcode = ReadMemory(0xF8000530);
	//LogDebug("pss_idcode = %08x\n", pss_idcode);

	//Read MCTRL
	uint32_t mctrl = ReadMemory(0xF8007080);
	LogDebug("mctrl = %08x\n", mctrl);

	//Set MIO7 (MIO LED) to output
	m_ap->GetDebugPort()->WriteMemory(0xf800071c, 0x00000600);	//sclr.MIO_PIN_07
	m_ap->GetDebugPort()->WriteMemory(0xe000a204, 0x00000080);	//gpio.XGPIOPS_DIRM_OFFSET
	m_ap->GetDebugPort()->WriteMemory(0xe000a208, 0x00000080);	//gpio.XGPIOPS_OUTEN_OFFSET
	for(int i=0; i<10; i++)
	{
		LogDebug("toggle\n");
		m_ap->GetDebugPort()->WriteMemory(0xe000a040, 0x00000080);	//gpio.XGPIOPS_DATA_OFFSET
		usleep(500 * 1000);
		m_ap->GetDebugPort()->WriteMemory(0xe000a040, 0x00000000);	//gpio.XGPIOPS_DATA_OFFSET
		usleep(500 * 1000);
	}

	//MIO LED @ MIO7
	//MIO inputs at MIO50, 51
	//GPIO controller is at 0xe000a000
	//Input data (DATA_RO) is at +0x60 - 6c
	//

	//Read L0_SEL

	//Read the PC and dump the instruction at that address
	uint32_t pc = SampleProgramCounter();
	LogVerbose("PC = %08x\n", pc);
	//uint32_t value = ReadMemory(0xE0000000);//m_ap->ReadWord(0x80000000); //ReadMemory(0xFC000000);

	//LogDebug("    value = %08x\n", value);
}

string ARMCortexA9::GetDescription()
{
	char tmp[128];
	snprintf(
		tmp,
		sizeof(tmp),
		"ARM Cortex-A9 rev %d mod %d stepping %d",
		m_idreg.revnum,
		m_idreg.cust_mod,
		m_idreg.revand
		);

	return string(tmp);
}

void ARMCortexA9::PrintIDRegister(ARMv7DebugIDRegister did)
{
	LogVerbose("CPU rev %u variant %u\n", did.bits.revision, did.bits.variant);

	const char* arches[]=
	{
		"reserved 0",
		"ARMv6, v6 debug arch",
		"ARMv6, v6.1 debug arch",
		"ARMv7, v7 debug, full CP14",
		"ARMv7, v7 debug, only baseline cp14",
		"ARMv7, v7.1 debug",
		"reserved 6",
		"reserved 7",
		"reserved 8",
		"reserved 9",
		"reserved a",
		"reserved b",
		"reserved c",
		"reserved d",
		"reserved e",
		"reserved f"
	};

	LogVerbose("Arch %s\n", arches[did.bits.debug_arch_version]);


	if(did.bits.sec_ext)
	{
		LogVerbose("Security extensions\n");
		if(did.bits.sec_ext && did.bits.no_secure_halt)
			LogDebug("    (but no secure halt)\n");
	}
	if(did.bits.pcsr_legacy_addr)
		LogVerbose("PCSR is at legacy address\n");
	if(did.bits.has_dbgdevid)
		LogVerbose("Has debug device ID\n");
	//TODO: arch version
	LogVerbose("%d breakpoints (%d with context matching)\n",
		did.bits.bpoints_minus_one+1, did.bits.context_bpoints_minus_one+1);
	LogVerbose("%d watchpoints\n", did.bits.wpoints_minus_one + 1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// System memory access

uint32_t ARMCortexA9::ReadMemory(uint32_t addr)
{
	return m_ap->GetDebugPort()->ReadMemory(addr);
}
