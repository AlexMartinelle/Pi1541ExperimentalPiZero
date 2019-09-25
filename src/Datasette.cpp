#if defined(TAPESUPPORT)

#include "Datasette.h"
#include "ff.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
extern "C"
{
#include "rpiHardware.h"
}

namespace PiDevice
{
	template <typename TEnum>
	bool IsSet(TEnum a, TEnum  b)
	{
		return (static_cast<unsigned int>(a) & static_cast<unsigned int>(b)) != 0;
	}

	template <typename TEnum>
	void Set(TEnum a, bool state, TEnum& b)
	{
		b = static_cast<TEnum>(state
			? static_cast<unsigned int>(b) | static_cast<unsigned int>(a)
			: static_cast<unsigned int>(b) & ~static_cast<unsigned int>(a)
			);
	}

	Datasette::Datasette()
		:m_button(Button::None)
		, m_tapePos(0)
		, m_cyclesLeft(1)
		, m_halwayCycles(0)
		, m_tapVersion(0)
		, m_tapeLength(0)
		, m_tape(nullptr)
		, m_signalsToC64(SignalsToC64::None)
		, m_signalsFromC64(SignalsFromC64::None)
	{
	}

	Datasette::~Datasette()
	{
		free(m_tape);
	}

	bool Datasette::IsTapeImageExtension(const char* tapeImageName)
	{
		return GetTapeImageTypeViaExtension(tapeImageName) != TapeType::Unknown;
	}

	Datasette::TapeType Datasette::GetTapeImageTypeViaExtension(const char* tapeImageName)
	{
		char* ext = strrchr((char*)tapeImageName, '.');

		if (ext)
		{
			if (toupper((char)ext[1]) == 'T' && toupper((char)ext[2]) == 'A' && toupper((char)ext[3]) == 'P')
				return TapeType::TAP;
		}
		return TapeType::Unknown;
	}

	Datasette::ErrorState Datasette::LoadTape(const char* tapName)
	{
		char header[12];
		FIL fp;
		FRESULT res = f_open(&fp, tapName, FA_READ);
		UINT br = 0;
		if (res != FR_OK)
			return OnFileError(fp, ErrorState::FileNotFound);

		res = f_read(&fp, &header[0], 12, &br);
		if (res != FR_OK)
			return OnFileError(fp, ErrorState::FileNotTAP);

		if (strncmp("C64-TAPE-RAW", header, 12) != 0)
			return OnFileError(fp, ErrorState::FileNotTAP);

		res = f_read(&fp, &m_tapVersion, 1, &br);
		if (res != FR_OK)
			return OnFileError(fp, ErrorState::Error);

		unsigned int reserved;
		res = f_read(&fp, &reserved, 3, &br);
		if (res != FR_OK)
			return OnFileError(fp, ErrorState::Error);


		char tapeLength[4];
		res = f_read(&fp, &tapeLength[0], 4, &br);
		if (res != FR_OK)
			return OnFileError(fp, ErrorState::Error);

		m_tapeLength = tapeLength[3] << 24 | tapeLength[2] << 16 | tapeLength[1] << 8 | tapeLength[0];

		free(m_tape);
		m_tape = static_cast<unsigned char*>(malloc(m_tapeLength));

		res = f_read(&fp, m_tape, m_tapeLength, &br);
		if (res != FR_OK)
			return OnFileError(fp, ErrorState::TAPTooShort);

		return OnFileError(fp, ErrorState::None);
	}

	Datasette::ErrorState Datasette::SaveTape(const char* tapName)
	{
		//Not implemented
		return ErrorState::Error;
	}

	void Datasette::Reset()
	{
		m_tapePos = 0;
		m_cyclesLeft = 1;
		m_halwayCycles = 0;
		m_signalsToC64 = SignalsToC64::None;
		m_signalsFromC64 = SignalsFromC64::None;
	}

	int Datasette::Update()
	{
		switch (m_button)
		{
		case Button::Play:
			Play();
			break;
		case Button::Record:
			Record();
			break;
		case Button::Done:
			m_signalsToC64 = SignalsToC64::None;
			return -1;
		default:
			Set(SignalsToC64::Sense, false, m_signalsToC64);
			break;
		}
		if (m_tapeLength < 1)
			return -1;

		return m_tapePos;
	}

	Datasette::SignalsToC64 Datasette::Signals(SignalsFromC64 signals)
	{
		m_signalsFromC64 = signals;
		return m_signalsToC64;
	}

	bool Datasette::Play()
	{
		Set(SignalsToC64::Sense, true, m_signalsToC64);

		if (!IsSet(m_signalsFromC64, SignalsFromC64::Motor))
			return true;

		--m_cyclesLeft;
		
		Set(SignalsToC64::Read, m_cyclesLeft < m_halwayCycles, m_signalsToC64);

		if (m_cyclesLeft > 0)
			return true;

		Set(SignalsToC64::Read, false, m_signalsToC64);
		if (m_tapePos >= m_tapeLength)
			return true;

		auto b = m_tape[m_tapePos++];
		if (b == 0)
		{
			switch (m_tapVersion)
			{
			case 1:
				m_cyclesLeft = 0;
				for (int i = 0; i != 3; ++i)
				{
					b = m_tape[m_tapePos++];
					m_cyclesLeft += b << (i * 8);
				}
				break;
			default:
				m_cyclesLeft = 65535;
				break;
			}
		}
		else
		{
			m_cyclesLeft = 8 * b;
		}

		//About halfway the signal should go high
		m_halwayCycles = m_cyclesLeft / 2;
		return true;
	}

	bool Datasette::Record()
	{
		Set(SignalsToC64::Sense, true, m_signalsToC64);

		if (!IsSet(m_signalsFromC64, SignalsFromC64::Motor))
			return true;

		return false;
	}

	void Datasette::SetButton(Button button)
	{
		m_button = button;
	}

	Datasette::ErrorState Datasette::OnFileError(FIL& fp, ErrorState state)
	{
		f_close(&fp);
		return state;
	}
}

#endif