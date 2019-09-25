#pragma once

#if defined(TAPESUPPORT)
#include "ff.h"

namespace PiDevice
{
	class Datasette
	{
	public:
		enum class TapeType
		{
			Unknown,
			TAP
		};
		enum class Button
		{
			None,
			Play,
			Stop,
			Pause,
			Record,
			Done
		};
		enum class ErrorState
		{
			None,
			FileNotFound,
			FileNotTAP,
			Error,
			TAPTooShort
		};
		enum class SignalsToC64
		{
			None,
			Read = 1,
			Sense = 2
		};
		enum class SignalsFromC64
		{
			None,
			Write = 1,
			Motor = 2
		};
		
		Datasette();
		~Datasette();

		ErrorState LoadTape(const char* tapName);
		ErrorState SaveTape(const char* tapName);
		void Reset();
		int Update();
		SignalsToC64 Signals(SignalsFromC64 signals);
		void SetButton(Button button);

		static TapeType GetTapeImageTypeViaExtension(const char* tapeImageName);
		static bool IsTapeImageExtension(const char* tapeImageName);
	private:
		bool Play();
		bool Record();
		ErrorState OnFileError(FIL& fp, ErrorState state);

		Button m_button;
		int m_tapePos;
		int m_cyclesLeft;
		int m_halwayCycles;
		int m_tapVersion;
		int m_tapeLength;
		unsigned char* m_tape;
		SignalsToC64 m_signalsToC64;
		SignalsFromC64 m_signalsFromC64;
	};
}
#endif
