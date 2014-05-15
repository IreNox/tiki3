
#include "tiki/entitysystem/entitysystem.hpp"

namespace tiki
{
	EntitySystem::EntitySystem()
	{
	}

	EntitySystem::~EntitySystem()
	{
	}

	bool EntitySystem::create( const EntitySystemParameters& parameters )
	{
		if ( !m_typeRegister.create( parameters.typeRegisterMaxCount ) )
		{
			dispose();
			return false;
		}

		if ( !m_storage.create( parameters.storageChunkSize, parameters.storageChunkCount, m_typeRegister ) )
		{
			dispose();
			return false;
		}

		if ( !m_pools.create( parameters.entityPools.getBegin(), parameters.entityPools.getCount() ) )
		{
			dispose();
			return false;
		}

		uint entityCapacity = 0u;
		for (uint i = 0u; i < m_pools.getCount(); ++i)
		{
			entityCapacity += m_pools[ i ].poolSize;
		}

		if ( !m_entities.create( entityCapacity ) )
		{
			dispose();
			return false;
		}

		return true;
	}

	void EntitySystem::dispose()
	{
		m_storage.dispose();
		m_typeRegister.dispose();
	}

	bool EntitySystem::registerComponentType( ComponentBase* pComponent )
	{
		const ComponentTypeId typeId = m_typeRegister.registerType( pComponent );
		if ( typeId != InvalidComponentTypeId )
		{
			// todo
			return true;
		}

		return false;
	}

	void EntitySystem::unregisterComponentType( ComponentBase* pComponent )
	{
		const ComponentTypeId typeId = InvalidComponentTypeId; // todo
		m_typeRegister.unregisterType( typeId );
	}

	EntityId EntitySystem::createEntityFromTemplate( const EntityTemplate& entityTemplate )
	{
		return InvalidEntityId;
	}

	void EntitySystem::destroyEntity( EntityId entityId )
	{

	}

	const ComponentState* EntitySystem::getFirstComponentOfEntity( EntityId entityId ) const
	{
		return nullptr;
	}

	const ComponentState* EntitySystem::getFirstComponentOfEntityAndType( EntityId entityId, ComponentTypeId typeId ) const
	{
		return nullptr;
	}
}