#pragma once
#ifndef __TIKI_STATICMODELCOMPONENT_INITDATA_HPP_INCLUDED__
#define __TIKI_STATICMODELCOMPONENT_INITDATA_HPP_INCLUDED__

#include "tiki/base/reflection.hpp"

#include "tiki/base/structs.hpp"

namespace tiki
{
	TIKI_REFLECTION_STRUCT(
		TransformComponentInitData,
		TIKI_REFLECTION_FIELD( float3, position )
		TIKI_REFLECTION_FIELD( float4, rotation )
	);
}

#endif // __TIKI_STATICMODELCOMPONENT_INITDATA_HPP_INCLUDED__
