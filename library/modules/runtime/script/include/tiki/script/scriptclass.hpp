#pragma once
#ifndef TIKI_SCRIPTOBJECT_HPP_INCLUDED
#define TIKI_SCRIPTOBJECT_HPP_INCLUDED

#include "tiki/base/types.hpp"
#include "tiki/script/scriptinstance.hpp"

struct lua_State;

namespace tiki
{
	class ScriptContext;	

	typedef int( *ScriptWrapperFunc )( lua_State* pState );

	struct ScriptMethod
	{
		const char*			pMethodName;
		ScriptWrapperFunc	pMethodFunc;
	};

	class ScriptClass
	{
		TIKI_NONCOPYABLE_CLASS( ScriptClass );

	protected:

						ScriptClass();
						~ScriptClass();

		bool			create( ScriptContext& context, const char* pName, const ScriptMethod* pMethods, uint methodCount );
		void			dispose();

		ScriptInstance	registerInstance( void* pInstance );
		void			unregisterInstance( ScriptInstance instance );

	private:

		ScriptContext*	m_pContext;
		const char*		m_pName;
	};
}

#endif // TIKI_SCRIPTOBJECT_HPP_INCLUDED
