
#include "tiki/gamestates/teststate.hpp"

#include "tiki/base/timer.hpp"
#include "tiki/framework/framework.hpp"
#include "tiki/gamestates/applicationstate.hpp"
#include "tiki/graphics/font.hpp"
#include "tiki/graphics/graphicscontext.hpp"
#include "tiki/graphics/graphicssystem.hpp"
#include "tiki/graphics/model.hpp"
#include "tiki/graphics/texture.hpp"
#include "tiki/graphics/graphicstypes.hpp"
#include "tiki/input/inputsystem.hpp"
#include "tiki/math/camera.hpp"
#include "tiki/math/projection.hpp"
#include "tiki/math/quaternion.hpp"
#include "tiki/math/rectangle.hpp"
#include "tiki/renderer/renderercontext.hpp"
#include "tiki/resource/resourcemanager.hpp"

namespace tiki
{
	struct FallbackVertex
	{
		float3	position;
		float4	color;
		float2	texCoord;
	};
	
	void TestState::create( ApplicationState* pParentState )
	{
		m_pParentState	= pParentState;

		m_drawPlayer		= false;
		m_enableBloom		= true;
		m_enableAsciiMode	= true;
		m_gbufferIndex		= -1;
		m_enableMouseCamera	= false;
		m_cameraSpeed		= 1.0f;

		vector::clear( m_cameraRotation );
		vector::clear( m_leftStickState );
		vector::clear( m_rightStickState );
	}

	void TestState::dispose()
	{
		m_pParentState = nullptr;
	}

	TransitionState TestState::processTransitionStep( size_t currentStep, bool isCreating, bool isInital )
	{
		switch ( currentStep )
		{
		case TestStateTransitionSteps_Initialize:
			if ( isCreating )
			{
				PostProcessAsciiParameters asciiParameters;	
				TIKI_VERIFY( m_ascii.create(
					framework::getGraphicsSystem(),
					framework::getResourceManager(),
					asciiParameters
				) );

				PostProcessBloomParameters bloomParameters;	
				bloomParameters.width		= framework::getGraphicsSystem().getBackBuffer().getWidth() / 2u;
				bloomParameters.height		= framework::getGraphicsSystem().getBackBuffer().getHeight() / 2u;
				bloomParameters.passCount	= 6u;
				TIKI_VERIFY( m_bloom.create(
					framework::getGraphicsSystem(),
					framework::getResourceManager(),
					bloomParameters
				) );

				return TransitionState_Finish;
			}
			else
			{
				m_bloom.dispose(
					framework::getGraphicsSystem(),
					framework::getResourceManager()
				);

				m_ascii.dispose(
					framework::getGraphicsSystem(),
					framework::getResourceManager()
				);

				return TransitionState_Finish;
			}
			//break;

		case TestStateTransitionSteps_LoadResources:
			if ( isCreating )
			{
				if ( isInital )
				{
					m_pFont				= framework::getResourceManager().loadResource< Font >( "debug.font" );
					m_pModelBoxes		= framework::getResourceManager().loadResource< Model >( "test_scene.model" );
					m_pModelPlane		= framework::getResourceManager().loadResource< Model >( "plane.model" );
					m_pModelPlayer		= framework::getResourceManager().loadResource< Model >( "player.model" );
					m_pAnimationPlayer	= framework::getResourceManager().loadResource< Animation >( "player.run.animation" );
					TIKI_ASSERT( m_pFont != nullptr );
					TIKI_ASSERT( m_pModelBoxes != nullptr );
					TIKI_ASSERT( m_pModelPlane != nullptr );
					TIKI_ASSERT( m_pModelPlayer != nullptr );
					TIKI_ASSERT( m_pAnimationPlayer != nullptr );

					m_animationData.create( m_pModelPlayer->getHierarchy()->getJointCount() );
					m_skinningData.matrices.create( framework::getGraphicsSystem(), sizeof( GraphicsMatrix44 ) * 256u );

					m_immediateRenderer.create( framework::getGraphicsSystem(), framework::getResourceManager() );

					return TransitionState_Finish;
				}
				else
				{
					// TODO: make resource manager async

					return TransitionState_Error;
				}
			}
			else
			{
				TIKI_ASSERT( isInital );

				framework::getResourceManager().unloadResource( m_pAnimationPlayer );
				framework::getResourceManager().unloadResource( m_pModelPlayer );
				framework::getResourceManager().unloadResource( m_pModelBoxes );
				framework::getResourceManager().unloadResource( m_pModelPlane );
				framework::getResourceManager().unloadResource( m_pFont );

				m_skinningData.matrices.dispose( framework::getGraphicsSystem() );
				m_animationData.dispose();

				m_immediateRenderer.dispose( framework::getGraphicsSystem(), framework::getResourceManager() );

				return TransitionState_Finish;
			}
			//break;

		case TestStateTransitionSteps_SetRendererValues:
			{
				m_pGameRenderer = &m_pParentState->getGameRenderer();

				if ( isCreating )
				{
					FrameData& frameData = m_pGameRenderer->getFrameData();
					frameData.nearPlane		= 0.001f;
					frameData.farPlane		= 100.0f;

					RendererContext& rendererContext = m_pGameRenderer->getRendererContext();

					const Vector3 cameraPosition = { 0.0f, 0.0f, 1.0f };
					frameData.mainCamera.create( cameraPosition, Quaternion::identity );

					m_fallbackRenderEffect.create( rendererContext, framework::getGraphicsSystem(), framework::getResourceManager() );
					m_sceneRenderEffect.create( rendererContext, framework::getGraphicsSystem(), framework::getResourceManager() );

					m_pGameRenderer->registerRenderEffect( &m_fallbackRenderEffect );
					m_pGameRenderer->registerRenderEffect( &m_sceneRenderEffect );

					m_debugGui.create( framework::getGraphicsSystem(), framework::getResourceManager(), 16u );
					m_testWindow.create( m_debugGui );
					m_lightingWindow.create( m_debugGui );

					m_testWindow.setRectangle( Rectangle( 20.0, 40.0f, 200.0f, 400.0f ) );
					m_lightingWindow.setRectangle( Rectangle( 1000.0, 100.0f, 250.0f, 100.0f ) );

					return TransitionState_Finish;
				}
				else
				{
					m_lightingWindow.dispose();
					m_testWindow.dispose();
					m_debugGui.dispose( framework::getGraphicsSystem(), framework::getResourceManager() );

					m_pGameRenderer->unregisterRenderEffect( &m_sceneRenderEffect );
					m_pGameRenderer->unregisterRenderEffect( &m_fallbackRenderEffect );
					m_sceneRenderEffect.dispose( framework::getGraphicsSystem(), framework::getResourceManager() );
					m_fallbackRenderEffect.dispose( framework::getGraphicsSystem(), framework::getResourceManager() );

					return TransitionState_Finish;
				}
			}
			//break;

		default:
			break;
		}

		TIKI_BREAK( "no result\n" );
		return TransitionState_Error;
	}

	void TestState::update()
	{
		const float timeDelta = (float)framework::getFrameTimer().getElapsedTime();

		FrameData& frameData = m_pGameRenderer->getFrameData();

		Vector3 cameraPosition = frameData.mainCamera.getPosition();
		Quaternion cameraRotation;

		// rotate camera
		{
			Vector2 rotation = m_rightStickState;
			if ( m_enableMouseCamera )
			{
				vector::clear( m_rightStickState );
			}
			vector::scale( rotation, timeDelta * 2.0f );

			m_cameraRotation.y = f32::clamp( m_cameraRotation.y, -f32::piOver2, f32::piOver2 );

			vector::add( m_cameraRotation, rotation );
			quaternion::fromYawPitchRoll( cameraRotation, m_cameraRotation.x, m_cameraRotation.y, 0.0f );
		}

		// move camera
		{
			Vector3 cameraForward;
			Vector3 cameraRight;
			quaternion::getForward( cameraForward, cameraRotation );
			quaternion::getRight( cameraRight, cameraRotation );

			vector::scale( cameraForward,	m_leftStickState.y * timeDelta * m_cameraSpeed );
			vector::scale( cameraRight,		m_leftStickState.x * timeDelta * m_cameraSpeed );

			vector::add( cameraPosition, cameraForward );
			vector::add( cameraPosition, cameraRight );
		}

		frameData.mainCamera.setTransform( cameraPosition, cameraRotation );

		DirectionalLightData& directionalLight = frameData.directionalLights.push();
		vector::set( directionalLight.direction, 0.941176471f, 0.235294118f, 0.0f );
		directionalLight.color = TIKI_COLOR_WHITE;

		const uint pointLightCount = 4u;
		const Color s_colors[ 4u ] =
		{
			TIKI_COLOR_RED,
			TIKI_COLOR_GREEN,
			TIKI_COLOR_BLUE,
			TIKI_COLOR_YELLOW
		};

		const float timeValue = (float)framework::getFrameTimer().getTotalTime() / 10.0f;
		//for (uint i = 0u; i < pointLightCount; ++i)
		//{
		//	const float value = ( ( f32::twoPi / pointLightCount ) * i ) + ( timeValue * -5.0f );

		//	PointLightData& pointLight = frameData.pointLights.push();
		//	vector::set( pointLight.position, sinf( value ) * 0.5f, 0.0f, cosf( value ) * 0.5f );
		//	pointLight.color = s_colors[ i ];
		//	pointLight.range = 2.0f;
		//}

		//PointLightData& pointLight = frameData.pointLights.push();
		//vector::set( pointLight.position, 0.0f, 0.0f, 0.0f );
		//pointLight.color	= TIKI_COLOR_WHITE;
		//pointLight.range	= 2.0f;

		//SpotLightData& spotLight = frameData.spotLights.push();
		//vector::set( spotLight.position, 1.0f, 1.0f, 0.0f );
		//vector::set( spotLight.direction, 0.5f, 0.5f, 0.0f );
		//spotLight.color = TIKI_COLOR_BLUE;
		//spotLight.range = 5.0f;
		//spotLight.theta = 0.174532925f;
		//spotLight.phi	= 1.22173048f;

		m_pAnimationPlayer->sample( m_animationData.getData(), m_animationData.getCount(), *m_pModelPlayer->getHierarchy(), timeValue );

		m_lightingWindow.fillFrameData( frameData );

		Matrix43 mtx = Matrix43::identity;
		mtx.pos.y = -0.1f;
		m_pGameRenderer->queueModel( m_pModelPlane, &mtx );

		matrix::createIdentity( mtx );
		matrix::createRotationY( mtx.rot, timeValue );
		if ( m_drawPlayer )
		{
			const SkinningData* pSkinningData = &m_skinningData;
			m_pGameRenderer->queueModel( m_pModelPlayer, &mtx, &pSkinningData );

		}
		else
		{
			m_pGameRenderer->queueModel( m_pModelBoxes, &mtx );
		}

		m_debugGui.update();
	}

	void TestState::render( GraphicsContext& graphicsContext )
	{
		Matrix44 matrices[ 256u ];
		//AnimationJoint::fillJointArrayFromHierarchy( m_animationData.getData(), m_animationData.getCount(), *m_pModelPlayer->getHierarchy() );
		AnimationJoint::buildPoseMatrices( matrices, TIKI_COUNT( matrices ), m_animationData.getData(), *m_pModelPlayer->getHierarchy() );

		GraphicsMatrix44* pShaderConstants = static_cast< GraphicsMatrix44* >( graphicsContext.mapBuffer( m_skinningData.matrices ) );
		for (uint i = 0u; i < m_animationData.getCount(); ++i)
		{
			createGraphicsMatrix44( pShaderConstants[ i ], matrices[ i ] );
		} 
		graphicsContext.unmapBuffer( m_skinningData.matrices );

		m_ascii.render( graphicsContext, m_pGameRenderer->getFrameData(), m_pGameRenderer->getRendererContext() );
		m_bloom.render( graphicsContext, m_pGameRenderer->getAccumulationBuffer(), m_pGameRenderer->getGeometryBufferBxIndex( 2u ) );

		const float timeDelta = (float)framework::getFrameTimer().getElapsedTime();
		const string frameRate = formatString( " FPS: %.2f", 1.0f / timeDelta );

		graphicsContext.clear( graphicsContext.getBackBuffer(), TIKI_COLOR_BLACK );

		m_immediateRenderer.beginRendering( graphicsContext );
		m_immediateRenderer.beginRenderPass();
			
		if ( m_enableAsciiMode )
		{
			const Rectangle rect = Rectangle( 0.0f, 0.0f, (float)m_ascii.getResultData().getWidth(), (float)m_ascii.getResultData().getHeight() );
			m_immediateRenderer.drawTexturedRectangle( m_ascii.getResultData(), rect );
		}
		else if ( m_gbufferIndex != -1 )
		{
			const Rectangle rect = Rectangle( 0.0f, 0.0f, (float)m_ascii.getDownSampleData().getWidth()*5, (float)m_ascii.getDownSampleData().getHeight()*5 );
			m_immediateRenderer.drawTexturedRectangle( m_ascii.getDownSampleData(), rect );

			//const TextureData& texture = m_pGameRenderer->getGeometryBufferBxIndex( m_gbufferIndex );
			//const Rectangle rect = Rectangle( 0.0f, 0.0f, (float)texture.getWidth(), (float)texture.getHeight() );
			//m_immediateRenderer.drawTexturedRectangle( &texture, rect );
		}
		else
		{
			const TextureData& texture = m_pGameRenderer->getAccumulationBuffer();
			const Rectangle rect = Rectangle( 0.0f, 0.0f, (float)texture.getWidth(), (float)texture.getHeight() );
			m_immediateRenderer.drawTexturedRectangle( texture, rect );
		}

		if ( m_enableBloom )
		{
			const Rectangle rect = Rectangle( 0.0f, 0.0f, (float)m_bloom.getResultData().getWidth() * 2.0f, (float)m_bloom.getResultData().getHeight() * 2.0f );
			m_immediateRenderer.drawTexturedRectangle( m_bloom.getResultData(), rect );
		}

		//const Rectangle rect2 = Rectangle( 50.0f, 50.0f, (float)m_pFont->getTextureData().getWidth(), (float)m_pFont->getTextureData().getHeight() );
		//m_immediateRenderer.drawTexture( &m_pFont->getTextureData(), rect2 );

		m_immediateRenderer.drawText( Vector2::zero, *m_pFont, frameRate.cStr(), TIKI_COLOR_GREEN );
		
		m_immediateRenderer.endRenderPass();
		m_immediateRenderer.endRendering();

		m_debugGui.render( graphicsContext );
	}

	bool TestState::processInputEvent( const InputEvent& inputEvent )
	{
		if ( m_debugGui.processInputEvent( inputEvent ) )
		{
			return true;
		}

		if ( inputEvent.eventType == InputEventType_Controller_StickChanged )
		{
			switch ( inputEvent.data.controllerStick.stickIndex )
			{
			case 0u:
				m_leftStickState.x	= inputEvent.data.controllerStick.xState;
				m_leftStickState.y	= inputEvent.data.controllerStick.yState;
				break;

			case 1u:
				m_rightStickState.x	= inputEvent.data.controllerStick.xState;
				m_rightStickState.y	= inputEvent.data.controllerStick.yState;
				break;
			}

			return true;
		}
		else if ( inputEvent.eventType == InputEventType_Keyboard_Down )
		{
			switch ( inputEvent.data.keybaordKey.key )
			{
			case KeyboardKey_F1:
				m_debugGui.setActive( !m_debugGui.getActive() );
				return true;

			case KeyboardKey_V:
				{
#if TIKI_DISABLED( TIKI_BUILD_MASTER )
					VisualizationMode visualizationMode = (VisualizationMode)( m_pGameRenderer->getVisualizationMode() + 1u );
					if ( visualizationMode == VisualizationMode_Count )
					{
						visualizationMode = VisualizationMode_Invalid;
					}
					m_pGameRenderer->setVisualizationMode( visualizationMode );
#endif
				}
				return true;

			case KeyboardKey_G:
				m_gbufferIndex++;
				if ( m_gbufferIndex > 2 ) m_gbufferIndex = -1;
				return true;

			case KeyboardKey_X:
				m_enableAsciiMode = !m_enableAsciiMode;
				return true;

			case KeyboardKey_B:
				m_enableBloom = !m_enableBloom;
				return true;

			default:
				break;
			}

			if ( m_enableMouseCamera )
			{
				switch ( inputEvent.data.keybaordKey.key )
				{
				case KeyboardKey_W:
					m_leftStickState.y = 1.0f;
					break;

				case KeyboardKey_A:
					m_leftStickState.x = -1.0f;
					break;

				case KeyboardKey_S:
					m_leftStickState.y = -1.0f;
					break;

				case KeyboardKey_D:
					m_leftStickState.x = 1.0f;
					break;

				case KeyboardKey_LeftShift:
				case KeyboardKey_RightShift:
					m_cameraSpeed = 10.0f;
					break;

				case KeyboardKey_Space:
					m_drawPlayer = !m_drawPlayer;
					break;
				}
			}
		}
		else if ( inputEvent.eventType == InputEventType_Keyboard_Up )
		{
			if ( m_enableMouseCamera )
			{
				switch ( inputEvent.data.keybaordKey.key )
				{
				case KeyboardKey_W:
					m_leftStickState.y = 0.0f;
					break;

				case KeyboardKey_A:
					m_leftStickState.x = 0.0f;
					break;

				case KeyboardKey_S:
					m_leftStickState.y = 0.0f;
					break;

				case KeyboardKey_D:
					m_leftStickState.x = 0.0f;
					break;

				case KeyboardKey_LeftShift:
				case KeyboardKey_RightShift:
					m_cameraSpeed = 1.0f;
					break;
				}
			}
		}
		else if ( inputEvent.eventType == InputEventType_Mouse_ButtonDown && inputEvent.data.mouseButton.button == MouseButton_Right )
		{
			m_enableMouseCamera = !m_enableMouseCamera;
		}
		else if ( inputEvent.eventType == InputEventType_Mouse_Moved && m_enableMouseCamera )
		{
			m_rightStickState.x = ( (float)inputEvent.data.mouseMoved.xOffset ) * 0.1f;
			m_rightStickState.y = ( (float)inputEvent.data.mouseMoved.yOffset ) * -0.1f;
		}

		return false;
	}
}