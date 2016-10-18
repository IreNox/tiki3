#pragma once
#ifndef __TIKI_COMPONENTBASE_HPP_INCLUDED__
#define __TIKI_COMPONENTBASE_HPP_INCLUDED__

#include "tiki/base/types.hpp"
#include "tiki/components/componentiterator.hpp"

namespace tiki
{
	class ComponentBase
	{
		TIKI_NONCOPYABLE_CLASS( ComponentBase );

	public:

								ComponentBase();
		virtual					~ComponentBase();

		void					registerComponent( ComponentTypeId typeId );
		void					unregisterComponent();

		virtual bool			initializeState( ComponentEntityIterator& componentIterator, ComponentState* pComponentState, const void* pComponentInitData ) TIKI_PURE;
		virtual void			disposeState( ComponentState* pComponentState ) TIKI_PURE;

		virtual crc32			getTypeCrc() const TIKI_PURE;
		virtual uint32			getStateSize() const TIKI_PURE;
		virtual const char*		getTypeName() const TIKI_PURE;

#if TIKI_ENABLED( TIKI_BUILD_DEBUG )
		virtual bool			checkIntegrity() const TIKI_PURE;
#endif

		ComponentTypeId			getTypeId() const { return m_registedTypeId; }
		
	protected:

		ComponentState*			m_pFirstComponentState;

		ComponentTypeId			m_registedTypeId;

	};

	template< typename TState, typename TInitData >
	class Component : public ComponentBase
	{
		TIKI_NONCOPYABLE_CLASS( Component );

	public:
		
		typedef TState									State;
		typedef TInitData								InitData;

		typedef ComponentTypeIterator< TState >			Iterator;
		typedef ComponentTypeIterator< const TState >	ConstIterator;

						Component() {}
		virtual			~Component() {}

		virtual bool	initializeState( ComponentEntityIterator& componentIterator, ComponentState* pComponentState, const void* pComponentInitData );
		virtual void	disposeState( ComponentState* pComponentState );
		
#if TIKI_ENABLED( TIKI_BUILD_DEBUG )
		virtual bool	checkIntegrity() const;
#endif

		Iterator		getIterator() const;
		ConstIterator	getConstIterator() const;

	protected:

		virtual bool	internalInitializeState( ComponentEntityIterator& componentIterator, TState* pComponentState, const TInitData* pComponentInitData ) TIKI_PURE;
		virtual void	internalDisposeState( TState* pComponentState ) TIKI_PURE;

	};
}

#include "../../../source/component.inl"

#endif // __TIKI_COMPONENTBASE_HPP_INCLUDED__
