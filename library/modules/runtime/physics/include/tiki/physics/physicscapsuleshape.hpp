#pragma once
#ifndef __TIKI_PHYSICSCAPSULESHAPE_HPP_INCLUDED__
#define __TIKI_PHYSICSCAPSULESHAPE_HPP_INCLUDED__

#include "tiki/base/types.hpp"
#include "tiki/physics/physicsbody.hpp"

#include "BulletCollision/CollisionShapes/btCapsuleShape.h"

namespace tiki
{
	struct Vector3;

	class PhysicsCapsuleShape : public PhysicsShape
	{
		TIKI_NONCOPYABLE_CLASS( PhysicsCapsuleShape );

	public:

						PhysicsCapsuleShape();
						~PhysicsCapsuleShape();

		void			create( float height, float radius );
		void			dispose();

		virtual void*	getNativeShape();

	private:

		btCapsuleShape	m_shape;

	};
}

#endif // __TIKI_PHYSICSCAPSULESHAPE_HPP_INCLUDED__