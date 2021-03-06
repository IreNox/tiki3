
#include "tiki/gameplay/gameclient.hpp"

#include "tiki/base/debugprop.hpp"
#include "tiki/components/entitytemplate.hpp"
#include "tiki/graphics/graphicscontext.hpp"
#include "tiki/math/basetypes.hpp"
#include "tiki/math/vector.hpp"
#include "tiki/renderer/gamerenderer.hpp"
#include "tiki/renderer/renderercontext.hpp"
#include "tiki/renderer/renderview.hpp"

#include "components.hpp"
#include "gamecomponents.hpp"

namespace tiki
{
	TIKI_DEBUGPROP_BOOL( s_useFreeCamera, "GameClient/UseFreeCamera", true );

	GameClient::GameClient()
	{
		m_pRenderView = nullptr;
	}

	GameClient::~GameClient()
	{
		TIKI_ASSERT( m_pRenderView == nullptr );
	}

	bool GameClient::create()
	{
		EntitySystemParameters entitySystemParams;
		entitySystemParams.typeRegisterMaxCount		= MaxTypeCount;
		entitySystemParams.storageChunkSize			= ChunkSize;
		entitySystemParams.storageChunkCount		= ChunkCount;

		EntityPool entityPools[] =
		{
			{    1u,   1u },	// player
			{ 1000u, 999u },	// enemies
			{ 2000u, 999u },	// level objects
			{ 3000u, 999u },	// static objects
			{ 4000u, 999u },	// projectiles
		};

		entitySystemParams.entityPools.create( entityPools, TIKI_COUNT( entityPools ) );

		if ( !m_entitySystem.create( entitySystemParams ) )
		{
			dispose();
			return false;
		}

		m_physicsWorld.create( vector::create( 0.0f, -9.81f, 0.0f ) );

		TIKI_VERIFY( m_transformComponent.create() );
		TIKI_VERIFY( m_entitySystem.registerComponentType( &m_transformComponent ) );

		TIKI_VERIFY( m_terrainComponent.create( m_transformComponent ) );
		TIKI_VERIFY( m_entitySystem.registerComponentType( &m_terrainComponent ) );

		TIKI_VERIFY( m_staticModelComponent.create( m_transformComponent ) );
		TIKI_VERIFY( m_entitySystem.registerComponentType( &m_staticModelComponent ) );

		TIKI_VERIFY( m_skinnedModelComponent.create( m_transformComponent ) );
		TIKI_VERIFY( m_entitySystem.registerComponentType( &m_skinnedModelComponent ) );

		TIKI_VERIFY( m_physicsBodyComponent.create( m_physicsWorld, m_transformComponent ) );
		TIKI_VERIFY( m_entitySystem.registerComponentType( &m_physicsBodyComponent ) );

		TIKI_VERIFY( m_physicsColliderComponent.create( m_physicsWorld ) );
		TIKI_VERIFY( m_entitySystem.registerComponentType( &m_physicsColliderComponent ) );

		TIKI_VERIFY( m_physicsCharacterControllerComponent.create( m_physicsWorld, m_transformComponent ) );
		TIKI_VERIFY( m_entitySystem.registerComponentType( &m_physicsCharacterControllerComponent ) );

		TIKI_VERIFY( m_playerControlComponent.create( m_transformComponent, m_physicsCharacterControllerComponent ) );
		TIKI_VERIFY( m_entitySystem.registerComponentType( &m_playerControlComponent ) );

		TIKI_VERIFY( m_lifeTimeComponent.create() );
		TIKI_VERIFY( m_entitySystem.registerComponentType( &m_lifeTimeComponent ) );

		TIKI_VERIFY( m_coinComponent.create( m_transformComponent, m_physicsBodyComponent, m_lifeTimeComponent, m_physicsWorld ) );
		TIKI_VERIFY( m_entitySystem.registerComponentType( &m_coinComponent ) );

		m_gameCamera.create(
			m_terrainComponent,
			vector::create( 0.0f, 5.0f, 0.0f )
		);

		m_freeCamera.create( Vector3::zero, Quaternion::identity );
		m_freeCamera.setMouseControl( true );

		RenderSceneParameters sceneParameters;
		if( !m_renderScene.create( sceneParameters ) )
		{
			dispose();
			return false;
		}

		RenderViewParameters viewParameters;
		m_pRenderView = m_renderScene.addView( viewParameters );
		
		return true;
	}

	void GameClient::dispose()
	{
		m_renderScene.removeView( m_pRenderView );
		m_renderScene.dispose();

		m_entitySystem.update(); // to dispose all entities

		m_freeCamera.dispose();
		m_gameCamera.dispose();

		m_entitySystem.unregisterComponentType( &m_coinComponent );
		m_entitySystem.unregisterComponentType( &m_lifeTimeComponent );
		m_entitySystem.unregisterComponentType( &m_playerControlComponent );
		m_entitySystem.unregisterComponentType( &m_physicsCharacterControllerComponent );
		m_entitySystem.unregisterComponentType( &m_physicsColliderComponent );
		m_entitySystem.unregisterComponentType( &m_physicsBodyComponent );
		m_entitySystem.unregisterComponentType( &m_skinnedModelComponent );
		m_entitySystem.unregisterComponentType( &m_staticModelComponent );
		m_entitySystem.unregisterComponentType( &m_terrainComponent );
		m_entitySystem.unregisterComponentType( &m_transformComponent );

		m_coinComponent.dispose();
		m_lifeTimeComponent.dispose();
		m_playerControlComponent.dispose();
		m_physicsCharacterControllerComponent.dispose();
		m_physicsColliderComponent.dispose();
		m_physicsBodyComponent.dispose();
		m_skinnedModelComponent.dispose();
		m_staticModelComponent.dispose();
		m_terrainComponent.dispose();
		m_transformComponent.dispose();

		m_physicsWorld.dispose();

		m_entitySystem.dispose();
	}
	
	EntityId GameClient::createPlayerEntity( const Model* pModel, const Vector3& position )
	{
		TransformComponentInitData transformInitData;
		createFloat3( transformInitData.position, position.x, position.y, position.z );
		createFloat4( transformInitData.rotation, 0.0f, 0.0f, 0.0f, 1.0f );
		createFloat3( transformInitData.scale, 1.0f, 1.0f, 1.0f );

		StaticModelComponentInitData modelInitData;
		modelInitData.model = pModel;

		PhysicsCharacterControllerComponentInitData controllerInitData;
		createFloat3( controllerInitData.position, position.x, position.y, position.z );
		controllerInitData.gravity				= 0.00001f;
		controllerInitData.shape.type			= PhysicsShapeType_Capsule;
		controllerInitData.shape.capsuleRadius	= 0.5f;
		controllerInitData.shape.capsuleHeight	= 1.0f;

		PlayerControlComponentInitData playerControlInitData;
		playerControlInitData.speed = 0.1f;

		EntityTemplateComponent entityComponents[] =
		{
			{ m_transformComponent.getTypeCrc(),					&transformInitData },
			{ m_physicsCharacterControllerComponent.getTypeCrc(),	&controllerInitData },
			{ m_staticModelComponent.getTypeCrc(),					&modelInitData },
			{ m_playerControlComponent.getTypeCrc(),				&playerControlInitData }
		};

		EntityTemplate entityTemplate;
		entityTemplate.components.create( entityComponents, TIKI_COUNT( entityComponents ) );

		const EntityId result = m_entitySystem.createEntityFromTemplate( 0u, entityTemplate );
		entityTemplate.components.dispose();
		return result;
	}

	EntityId GameClient::createModelEntity( const Model* pModel, const Vector3& position )
	{
		TransformComponentInitData transformInitData;
		createFloat3( transformInitData.position, position.x, position.y, position.z );
		createFloat4( transformInitData.rotation, 0.0f, 0.0f, 0.0f, 1.0f );
		createFloat3( transformInitData.scale, 1.0f, 1.0f, 1.0f );

		StaticModelComponentInitData modelInitData;
		modelInitData.model = pModel;

		EntityTemplateComponent entityComponents[] =
		{
			{ m_transformComponent.getTypeCrc(), &transformInitData },
			{ m_staticModelComponent.getTypeCrc(), &modelInitData }
		};

		EntityTemplate entityTemplate;
		entityTemplate.components.create( entityComponents, TIKI_COUNT( entityComponents ) );

		const EntityId result = m_entitySystem.createEntityFromTemplate( 1u, entityTemplate );
		entityTemplate.components.dispose();
		return result;
	}
	
	EntityId GameClient::createBoxEntity( const Model* pModel, const Vector3& position )
	{
		TransformComponentInitData transformInitData;
		createFloat3( transformInitData.position, position.x, position.y, position.z );
		createFloat4( transformInitData.rotation, 0.0f, 0.0f, 0.0f, 1.0f );
		createFloat3( transformInitData.scale, 1.0f, 1.0f, 1.0f );

		StaticModelComponentInitData modelInitData;
		modelInitData.model = pModel;

		PhysicsBodyComponentInitData bodyInitData;
		createFloat3( bodyInitData.position, position.x, position.y, position.z );
		bodyInitData.mass			= 100.0f;
		bodyInitData.freeRotation	= true;
		bodyInitData.shape.type		= PhysicsShapeType_Box;
		createFloat3( bodyInitData.shape.boxSize, 1.0f, 1.0f, 1.0f );

		LifeTimeComponentInitData lifeTimeInitData;
		lifeTimeInitData.lifeTimeInMs = 60000;

		EntityTemplateComponent entityComponents[] =
		{
			{ m_transformComponent.getTypeCrc(),	&transformInitData },
			{ m_physicsBodyComponent.getTypeCrc(),	&bodyInitData },
			{ m_staticModelComponent.getTypeCrc(),	&modelInitData },
			{ m_lifeTimeComponent.getTypeCrc(),		&lifeTimeInitData }
		};

		EntityTemplate entityTemplate;
		entityTemplate.components.create( entityComponents, TIKI_COUNT( entityComponents ) );

		const EntityId result = m_entitySystem.createEntityFromTemplate( 1u, entityTemplate );
		entityTemplate.components.dispose();
		return result;
	}
	
	EntityId GameClient::createCoinEntity( const Model* pModel, const Vector3& position )
	{
		TransformComponentInitData transformInitData;
		createFloat3( transformInitData.position, position.x, position.y, position.z );
		createFloat4( transformInitData.rotation, 0.0f, 0.0f, 0.0f, 1.0f );
		createFloat3( transformInitData.scale, 0.5f, 0.5f, 0.5f );

		StaticModelComponentInitData modelInitData;
		modelInitData.model = pModel;

		PhysicsBodyComponentInitData bodyInitData;
		createFloat3( bodyInitData.position, position.x, position.y, position.z );
		bodyInitData.mass			= 100.0f;
		bodyInitData.freeRotation	= true;
		bodyInitData.shape.type		= PhysicsShapeType_Box;
		createFloat3( bodyInitData.shape.boxSize, 0.5f, 0.5f, 0.5f );

		LifeTimeComponentInitData lifeTimeInitData;
		lifeTimeInitData.lifeTimeInMs = 60000;

		CoinComponentInitData coinInitData;
		coinInitData.value = 1.0f;

		EntityTemplateComponent entityComponents[] =
		{
			{ m_transformComponent.getTypeCrc(),	&transformInitData },
			{ m_physicsBodyComponent.getTypeCrc(),	&bodyInitData },
			{ m_staticModelComponent.getTypeCrc(),	&modelInitData },
			{ m_lifeTimeComponent.getTypeCrc(),		&lifeTimeInitData },
			{ m_coinComponent.getTypeCrc(),			&coinInitData }
		};

		EntityTemplate entityTemplate;
		entityTemplate.components.create( entityComponents, TIKI_COUNT( entityComponents ) );

		const EntityId result = m_entitySystem.createEntityFromTemplate( 1u, entityTemplate );
		entityTemplate.components.dispose();
		return result;
	}

	EntityId GameClient::createTerrainEntity( const Model* pModel, const Vector3& position )
	{
		TransformComponentInitData transformInitData;
		createFloat3( transformInitData.position, position.x, position.y, position.z );
		createFloat4( transformInitData.rotation, 0.0f, 0.0f, 0.0f, 1.0f );
		createFloat3( transformInitData.scale, 1.0f, 1.0f, 1.0f );

		TerrainComponentInitData terrainInitData;
		terrainInitData.model = pModel;

		PhysicsColliderComponentInitData colliderInitData;
		createFloat3( colliderInitData.position, position.x, position.y + 0.005f, position.z );
		colliderInitData.shape.type = PhysicsShapeType_Box;
		createFloat3( colliderInitData.shape.boxSize, 100.00f, 0.01f, 100.0f );

		EntityTemplateComponent entityComponents[] =
		{
			{ m_transformComponent.getTypeCrc(), &transformInitData },
			{ m_physicsColliderComponent.getTypeCrc(), &colliderInitData },
			{ m_terrainComponent.getTypeCrc(), &terrainInitData }
		};

		EntityTemplate entityTemplate;
		entityTemplate.components.create( entityComponents, TIKI_COUNT( entityComponents ) );

		const EntityId result = m_entitySystem.createEntityFromTemplate( 1u, entityTemplate );
		entityTemplate.components.dispose();
		return result;
	}

	void GameClient::disposeEntity( EntityId entityId )
	{
		m_entitySystem.disposeEntity( entityId );
	}

	void GameClient::update( GameClientUpdateContext& updateContext )
	{
		const timems timeMs = timems( updateContext.timeDelta * 1000.0f );

		m_renderScene.clearState();

		m_entitySystem.update();

		Camera camera;
		camera.create( m_pRenderView->getCamera() );
		m_gameCamera.update( camera, updateContext.pTerrainState, updateContext.timeDelta );

		if( s_useFreeCamera )
		{
			m_freeCamera.update( m_pRenderView->getCamera(), updateContext.timeDelta );
		}
		else
		{
			m_pRenderView->getCamera().create( camera );
		}

		m_physicsWorld.update( updateContext.timeDelta );

		m_physicsCharacterControllerComponent.update();
		m_physicsBodyComponent.update();
		m_playerControlComponent.update( m_gameCamera, camera, updateContext.timeDelta );
		m_lifeTimeComponent.update( m_entitySystem, timeMs );
		m_coinComponent.update( updateContext.pPlayerCollider, updateContext.collectedCoins, updateContext.totalGameTime );

		m_transformComponent.update();
	}

	void GameClient::render( GameRenderer& gameRenderer, GraphicsContext& graphicsContext )
	{
		m_staticModelComponent.render( m_renderScene );
		m_skinnedModelComponent.render( m_renderScene );
		m_terrainComponent.render( m_renderScene );

		gameRenderer.renderView( graphicsContext, graphicsContext.getBackBuffer(), *m_pRenderView );
	}
	
	bool GameClient::processInputEvent( const InputEvent& inputEvent )
	{
		if( s_useFreeCamera )
		{
			return m_freeCamera.processInputEvent( inputEvent );
		}

		if ( m_playerControlComponent.processInputEvent( inputEvent ) )
		{
			return true;
		}

		return false;
	}
}