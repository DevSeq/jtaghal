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
	@brief Declaration of FTDIDriver
 */

#ifndef FTDIDriver_h
#define FTDIDriver_h

#ifdef HAVE_FTD2XX

#define BIT_MODE_RESET					0x00	/* Reset the MPSSE */
#define BIT_MODE_MPSSE					0x02	/* MPSSE mode */

/**
	@brief Common logic used by all FTDI adapters, regardless of transport layer selected

	This adapter queues up to 4096 bytes of command+data before comitting to hardware.

	GPIO pin mapping:

	Index	| Name
	--------|--------
		0	| GPIOL0 (ADBUS4)
		1	| GPIOL1 (ADBUS5)
		2	| GPIOL2 (ADBUS6)
		3	| GPIOL3 (ADBUS7)
		4	| GPIOH0 (ACBUS0)
		5	| GPIOH1 (ACBUS1)
		6	| GPIOH2 (ACBUS2)
		7	| GPIOH3 (ACBUS3)
		8	| GPIOH4 (ACBUS4)
		9	| GPIOH5 (ACBUS5)
		10	| GPIOH6 (ACBUS6)
		11	| GPIOH7 (ACBUS7)

	Supported layouts:

	Name | Example hardware | Pin configuration
	-----|------------------|--------------------
	hs1  | Digilent JTAG-HS1, Digilent JTAG-SMT2, azonenberg's usb-jtag-mini | ADBUS7 is active-high output enable
	hs2  | Digilent JTAG-HS2 | ADBUS7...5 are active-high output enable
	jtagkey | Amontec JTAGkey, Bus Blaster w/ JTAGkey compatible buffer | ADBUS4 is active-low output enable, ACBUS0 is TRST_N, ACBUS2 is active-low output enable for TRST_N


	\ingroup interfaces
 */
class FTDIDriver : public GPIOInterface
{
	//Construction / destruction
public:
	FTDIDriver(const std::string& serial, const std::string& layout);
	virtual ~FTDIDriver();

protected:
	void SharedCtorInit(uint32_t type, const std::string& layout);

	//Interface enumeration and discovery
public:
	virtual std::string GetName();
	virtual std::string GetSerial();
	virtual std::string GetUserID();
	virtual int GetFrequency();

	static bool IsMPSSECapable(int index);
	static int GetDefaultFrequency(int index);
	static std::string GetSerialNumber(int index);
	static std::string GetDescription(int index);
	static std::string GetAPIVersion();
	static int GetInterfaceCount();

	//Buffer management
public:
	virtual void Commit();

	//GPIO stuff
public:
	virtual void ReadGpioState();
	virtual void WriteGpioState();

	//Internal I/O helpers
protected:
	///@brief Cached name of this adapter
	std::string m_name;

	///@brief Cached serial number of this adapter
	std::string m_serial;

	///@brief Cached user ID of this adapter
	std::string m_userid;

	///@brief Cached clock frequency of this adapter
	int m_freq;

	///@brief Libftd2xx interface handle
	void* m_context;

	///@brief Buffer of data queued for the adapter, but not yet sent
	std::vector<unsigned char> m_writeBuffer;

	void SyncCheck();

	void ReadData(void* data, size_t bytesToRead);
	void WriteDataRaw(const void* data, size_t bytesToWrite);
	void WriteData(const void* data, size_t bytesToWrite);
	void WriteData(unsigned char cmd);

	//Internal constants
protected:
	enum MPSSE_Commands
	{
		MPSSE_TX_BYTES					= 0x19,
		MPSSE_TX_BITS					= 0x1b,
		MPSSE_TXRX_BYTES				= 0x39,
		MPSSE_TXRX_BITS					= 0x3b,
		MPSSE_TX_TMS_BITS				= 0x4b,
		MPSSE_TXRX_TMS_BITS				= 0x6b,
		MPSSE_SET_DATA_LOW				= 0x80,
		MPSSE_GET_DATA_LOW				= 0x81,
		MPSSE_SET_DATA_HIGH				= 0x82,
		MPSSE_GET_DATA_HIGH				= 0x83,
		MPSSE_DISABLE_LOOPBACK			= 0x85,
		MPSSE_SET_CLKDIV				= 0x86,
		MPSSE_FLUSH						= 0x87,
		MPSSE_DISABLE_DIV5				= 0x8a,
		MPSSE_DISABLE_3PHA 				= 0x8d,
		MPSSE_DUMMY_CLOCK_BITS			= 0x8e,
		MPSSE_DUMMY_CLOCK_BYTES			= 0x8f,
		MPSSE_DISABLE_ADAPTIVE_CLK		= 0x97,
		MPSSE_INVALID_COMMAND 			= 0xAA,		//Invalid command for resyncing
		MPSSE_INVALID_COMMAND_RESPONSE	= 0xFA
	};
};

#endif

#endif
