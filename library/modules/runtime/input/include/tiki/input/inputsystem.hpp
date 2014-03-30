#ifndef TIKI_INPUTSYSTEM_HPP__
#define TIKI_INPUTSYSTEM_HPP__

#include "tiki/base/fixedsizedarray.hpp"
#include "tiki/base/platform.hpp"
#include "tiki/base/types.hpp"
#include "tiki/input/inputdevice.hpp"
#include "tiki/input/inputevent.hpp"

#if TIKI_ENABLED( TIKI_PLATFORM_WIN )
#	include "../../../source/win/inputsystem_win.hpp"
#else
#	error Platform not supported
#endif

namespace tiki
{
	struct InputSystemParameters
	{
		WindowHandle windowHandle;
	};
	
	class InputSystem
	{
		TIKI_NONCOPYABLE_CLASS( InputSystem );
		friend struct FrameworkData;

	public:

		enum
		{
			MaxInputDeviceCount	= 8u,
			MaxInputEventCount	= 64u
		};

		typedef FixedSizedArray< InputDevice, MaxInputDeviceCount > InputDeviceArray;
		typedef FixedSizedArray< InputEvent, MaxInputEventCount > InputEventArray;

		bool					create( const InputSystemParameters& params );
		void					dispose();

		void					update();

		uint					getDeviceCount() const;
		InputDevice&			getDeviceByIndex( uint index ) const;

		uint					getEventCount() const;
		bool					popEvent( InputEvent& inputEvent );

		//bool					isKeyDown( Keys key ) const;
		//bool					isKeyUp( Keys key ) const;

		//bool					hasKeyPressed( Keys key ) const;
		//bool					hasKeyReleased( Keys key ) const;

		//bool					isButtonDown( MouseButtons button ) const;
		//bool					isButtonUp( MouseButtons button ) const;

		//bool					isButtonPressed( MouseButtons button ) const;
		//bool					isButtonReleased( MouseButtons button ) const;

		//const Vector2&			getMousePosition() const			{ return m_mousePosition; }
		//const Vector2&			getMousePositionNormalized() const	{ return m_mousePositionNormalized; }
		//const Vector2&			getMouseDeltaNormalized() const		{ return m_mouseDeltaNormalized; }

	private: // friends

								InputSystem();
								~InputSystem();
							
	private:
			
		InputDeviceArray		m_devices;
		InputEventArray			m_events;

		InputSystemPlatformData	m_platformData;

		//Vector2					m_mousePosition;
		//Vector2					m_mousePositionNormalized;
		//Vector2					m_mouseDeltaNormalized;

		void					connectDevice( const InputDevice& device );
		void					disconnectDevice( const InputDevice& device );

	};
}
#endif // TIKI_INPUTSYSTEM_HPP__