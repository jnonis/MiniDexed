//
// mididevice.h
//
// MiniDexed - Dexed FM synthesizer for bare metal Raspberry Pi
// Copyright (C) 2022  The MiniDexed Team
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#ifndef _displaybuffer_h
#define _displaybuffer_h

#include <circle/writebuffer.h>
#include <circle/string.h>
#include <circle/timer.h>
#include <string>
#include <cstring>

class CDisplayBufferDevice : public CWriteBufferDevice
{
public:
    CDisplayBufferDevice(CDevice *pDevice, size_t nBufferSize = 4096);
    ~CDisplayBufferDevice();
    void DisplayWrite(CString Msg);
    void Update();
    void ForceUpdate();
private:
#ifndef ARM_ALLOW_MULTI_CORE
	// Single core RPis
	static const unsigned DISPLAY_UPDATE_WRITE_TIME = 66; // around 15 Messages per second
	static const unsigned DISPLAY_BYTES_PER_CYCLE = 1; // will take several cycle to sent the message
    // Whit this time it will take 82ms or more to send the menssage, depending on the sound processing load
    static const unsigned DISPLAY_DISPLAY_TIME = 2;
#else
    // Multi core RPis
	static const unsigned DISPLAY_UPDATE_WRITE_TIME = 33; // around 30 Messages per second
    static const unsigned DISPLAY_BYTES_PER_CYCLE = 41; // enough to sent a message in one cycle
    // This time should be enough to send all the bytes (41) to the display.
    static const unsigned DISPLAY_DISPLAY_TIME = 33;
#endif

    bool m_bInitalized = false;
    CString m_DisplayWriteBuffer;
	bool m_bDisplayWriteUpdate;
	unsigned long m_nDisplayWriteUpdateTime;
	unsigned long m_nLastLCDUpdateTime;
};

#endif