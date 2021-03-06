#pragma once
#ifndef TIKI_BASEAPPLICATION_HPP_INCLUDED
#define TIKI_BASEAPPLICATION_HPP_INCLUDED

#include "tiki/base/types.hpp"
#include "tiki/graphics/graphicsrenderermode.hpp"

namespace tiki
{
	class GraphicsContext;
	class GraphicsSystem;
	class ImmediateRenderer;
	class InputSystem;
	class MainWindow;
	class ResourceManager;
	class Timer;
	class UiSystem;
	struct BaseApplicationkData;
	struct InputEvent;
	struct WindowEvent;

	struct BaseApplicationParamters
	{
		BaseApplicationParamters()
		{
			screenWidth			= 900;
			screenHeight		= 600;
			fullScreen			= false;
			graphicsMode		= GraphicsRendererMode_Hardware;

			pWindowTitle		= "TikiEngine 3.0";

			pGamebuildPath		= "gamebuild/";
		}

		uint					screenWidth;
		uint					screenHeight;
		bool					fullScreen;
		GraphicsRendererMode	graphicsMode;

		const char*					pWindowTitle;

		const char*					pGamebuildPath;
	};

	class BaseApplication
	{
		TIKI_NONCOPYABLE_CLASS( BaseApplication );

	public:

		int								run();

		MainWindow&						getMainWindow() const;
		GraphicsSystem&					getGraphicsSystem() const;
		ResourceManager&				getResourceManager() const;
		InputSystem&					getInputSystem() const;
		UiSystem&						getUiSystem() const;
		const Timer&					getFrameTimer() const;
		const ImmediateRenderer&		getImmediateRenderer() const;

	protected:

										BaseApplication();
		virtual							~BaseApplication();

		virtual void					fillBaseParameters( BaseApplicationParamters& parameters ) TIKI_PURE;

		virtual bool					initializeApplication() TIKI_PURE;
		virtual void					shutdownApplication() TIKI_PURE;

		virtual void					updateApplication( bool wantToShutdown ) TIKI_PURE;
		virtual void					renderApplication( GraphicsContext& graphicsContext ) const TIKI_PURE;

		virtual bool					processBaseInputEvent( const InputEvent& inputEvent ) TIKI_PURE;
		virtual void					processBaseWindowEvent( const WindowEvent& windowEvent ) TIKI_PURE;

		bool							m_running;

	private:

		bool							m_isInitialized;

		BaseApplicationParamters		m_parameters;
		BaseApplicationkData*			m_pBaseData;

		bool							initialize();
		void							shutdown();

		bool							initializePlatform();
		void							shutdownPlatform();
		void							updatePlatform();

		bool							initializeFramework();
		void							shutdownFramework();

		bool							frame();
	};
}

#endif // TIKI_BASEAPPLICATION_HPP_INCLUDED
