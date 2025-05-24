#include "displaybuffer.h"

CDisplayBufferDevice::CDisplayBufferDevice(CDevice *pDevice, unsigned nColumns, size_t nBufferSize) : CWriteBufferDevice(pDevice, nBufferSize)
{
	m_DisplayWriteBuffer.Append("");
	m_DisplayState.Append("");
	m_bDisplayWriteUpdate = false;
	m_nDisplayWriteUpdateTime = 0;
	m_nLastLCDUpdateTime = 0;
	m_nColumns = nColumns;
	m_nCursor = -1;
	m_nCursorState = -1;
	m_bInitalized = true;
}

CDisplayBufferDevice::~CDisplayBufferDevice()
{
}

void CDisplayBufferDevice::DisplayWrite(const char *pMenu, const char *pParam, const char *pValue,
		bool bArrowDown, bool bArrowUp)
{
	if (!m_bInitalized)
	{
		return;
	}

	CString Msg("");

	// first line
	Msg.Append(pParam);
	size_t nLen = strlen(pParam) + strlen(pMenu);
	if (nLen < m_nColumns)
	{
		for (unsigned i = m_nColumns - nLen; i > 0; i--)
		{
			Msg.Append(" ");
		}
	}
	Msg.Append(pMenu);

	// second line
	CString Value(" ");
	if (bArrowDown)
	{
		Value = "<"; // arrow left character
	}
	Value.Append(pValue);
	if (Value.GetLength() < m_nColumns - 1)
	{
		for (unsigned i = m_nColumns - Value.GetLength(); i > 0; i--)
		{
			if (bArrowUp && i == 1)
			{
				Value.Append(">"); // arrow right character
			}
			else
			{
				Value.Append(" ");
			}
		}
	}
	Msg.Append(Value);

	this->DisplayWrite(Msg);
}

void CDisplayBufferDevice::DisplayWrite(CString Msg)
{
	if (!m_bInitalized)
	{
		return;
	}
	// Check if the message has changed.
	// If message to display has not changed DO NOT updated the display.
	if (m_DisplayWriteBuffer != NULL && m_DisplayWriteBuffer.Compare(Msg) != 0)
	{
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
		if (m_bDisplayWriteUpdate)
		{
			CString message = PrepareMessage();
			CWriteBufferDevice::Write(message, strlen(message));
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

void CDisplayBufferDevice::SetCursor(int nCursor)
{
	m_nCursor = nCursor;
	m_bDisplayWriteUpdate = m_nCursor != m_nCursorState ? true : m_bDisplayWriteUpdate;
}

CString CDisplayBufferDevice::PrepareMessage()
{
	CString updateMsg("");
	if (m_DisplayState.GetLength() == 0) {
		updateMsg.Append("\E[H");
		updateMsg.Append(m_DisplayWriteBuffer);
		m_DisplayState = m_DisplayWriteBuffer;
		return updateMsg;
	}
	
	if (m_nCursor != m_nCursorState || m_nCursor >= 0)
	{
		// Disable cursor during update
		updateMsg.Append("\E[?25l");
		m_nCursorState = m_nCursor;
	}

	bool onchange = false;
	for (u_int8_t i = 0; i < m_DisplayState.GetLength(); i++)
	{
		if (m_DisplayState[i] != m_DisplayWriteBuffer[i])
		{
			if (!onchange)
			{
				// Move cursor
				int row = i / m_nColumns;
				int col = i < m_nColumns ? i : i - (m_nColumns * row);
				updateMsg.Append("\E[");
				updateMsg.Append(std::to_string(row + 1).c_str());
				updateMsg.Append(";");
				updateMsg.Append(std::to_string(col + 1).c_str());
				updateMsg.Append("H");
				onchange = true;
			}
			char temp[2];
			temp[0] = m_DisplayWriteBuffer[i];
			temp[1] = '\0';
			updateMsg.Append(temp);
		}
		else
		{
			onchange = false;
		}
	}

	if (m_nCursor >= 0)
	{
		// Enabled cursor again
		int row = m_nCursor / m_nColumns;
		int col = m_nCursor < (int) m_nColumns ? m_nCursor : m_nCursor - (m_nColumns * row);
		updateMsg.Append("\E[?25h\E[");
		updateMsg.Append(std::to_string(row + 1).c_str());
		updateMsg.Append(";");
		updateMsg.Append(std::to_string(col + 1).c_str());
		updateMsg.Append("H");
	}
	
	m_DisplayState = m_DisplayWriteBuffer;
	return updateMsg;
}