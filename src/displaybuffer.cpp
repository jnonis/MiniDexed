#include "displaybuffer.h"


CDisplayBufferDevice::CDisplayBufferDevice(CDevice *pDevice, size_t nBufferSize) : CWriteBufferDevice(pDevice, nBufferSize)
{
	m_DisplayWriteBuffer.Append(" ");
	m_bDisplayWriteUpdate = false;
	m_nDisplayWriteUpdateTime = 0;
	m_nLastLCDUpdateTime = 0;
	m_bInitalized = true;
}

CDisplayBufferDevice::~CDisplayBufferDevice()
{	
}

void CDisplayBufferDevice::DisplayWrite(CString Msg)
{
	if (!m_bInitalized) {
		return;
	}
    // Check if the message has changed.
	// If message to display has not changed DO NOT updated the display.
	if (m_DisplayWriteBuffer != NULL && m_DisplayWriteBuffer.Compare(Msg) != 0) {
		m_DisplayWriteBuffer = Msg;
		m_bDisplayWriteUpdate = true;
	}
}

void CDisplayBufferDevice::Update()
{
    // Limit display updates to avoid glitches on sigle core RPis
	// CWriteBufferDevice Write() add all the messages to an internal buffer and all those messagte will be send to the display.
	// That means, in case UI recive a lot of updates (like changing a number value) all those updates will be send to the display.
	// The internal buffer of CDisplayBufferDevice is overwritten, it does not keep values which are not valid at the moment
	// update the display.
	unsigned long nReadTime = CTimer::GetClockTicks() / (CLOCKHZ / 1000);
	if (nReadTime - m_nDisplayWriteUpdateTime > DISPLAY_UPDATE_WRITE_TIME)
	{
		// Only write the device buffer if the content has changed.
		if (m_bDisplayWriteUpdate) {
			CWriteBufferDevice::Write(m_DisplayWriteBuffer, strlen(m_DisplayWriteBuffer));
			m_bDisplayWriteUpdate = false;
		}
		m_nDisplayWriteUpdateTime = nReadTime;
	}
	// Limit the amount of cpu time to be used to send data to the display.
	// Depending on DISPLAY_DISPLAY_TIME and DISPLAY_BYTES_PER_CYCLE the messages will be sent completly or partially to the display.
	// In case that a message is sent partially the remaing bytes will be sent in the next cycle/s.
	if (nReadTime - m_nLastLCDUpdateTime > DISPLAY_DISPLAY_TIME)
	{
		// Limit amount of bytes to be send in this cyple
		CWriteBufferDevice::Update(DISPLAY_BYTES_PER_CYCLE);
        m_nLastLCDUpdateTime = nReadTime;
    }
}

void CDisplayBufferDevice::ForceUpdate()
{
	// Flush all the messages to the display
	CWriteBufferDevice::Update();
}