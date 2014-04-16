#pragma once
#ifndef TIKI_TESTSTATE_HPP__INCLUDED
#define TIKI_TESTSTATE_HPP__INCLUDED

#include "tiki/gameflow/gamestate.hpp"

#include "tiki/graphics/immediaterenderer.hpp"
#include "tiki/renderer/fallbackrendereffect.hpp"
#include "tiki/renderer/scenerendereffect.hpp"

#include "tiki/debugmenu/debugmenu.hpp"
#include "tiki/debugmenu/debugmenupage_debugprop.hpp"

#include "tiki/renderer/postascii.hpp"

namespace tiki
{
	class ApplicationState;
	class Font;
	class GameRenderer;
	class Model;
	class Texture;

	enum TestStateTransitionSteps
	{
		TestStateTransitionSteps_Initialize,
		TestStateTransitionSteps_LoadResources,
		TestStateTransitionSteps_SetRendererValues,

		TestStateTransitionSteps_Count
	};

	class TestState : public GameState
	{
		TIKI_NONCOPYABLE_WITHCTOR_CLASS( TestState );

	public:								

		void					create( ApplicationState* pParentState );
		void					dispose();

		virtual TransitionState	processTransitionStep( size_t currentStep, bool isCreating, bool isInital );

		virtual void			update();
		virtual void			render( GraphicsContext& graphicsContext );

		virtual bool			processInputEvent( const InputEvent& inputEvent );

	private:
		
		ApplicationState*			m_pParentState;

		const Font*					m_pFont;
		const Model*				m_pModel;

		GameRenderer*				m_pGameRenderer;
		FallbackRenderEffect		m_fallbackRenderEffect;
		SceneRenderEffect			m_sceneRenderEffect;

		ImmediateRenderer			m_immediateRenderer;

		Vector2						m_leftStickState;
		Vector2						m_rightStickState;

		DebugMenu					m_debugMenu;
		DebugMenuPageDebugProp		m_debugMenuPageDebugProp;

		PostProcessAscii			m_ascii;

	};
}

#endif // TIKI_TESTSTATE_HPP__INCLUDED
