#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> 
#include "MathLib\VectorTypes.h"

namespace Math 
{
	using Vector2 = glm::vec2;
	using Vector3 = glm::vec3;
	using Vector4 = glm::vec4;
}

// Custom Types

#include "MathLib\Types\Ray.h"

#include "MathLib\Types\Circle.h"
#include "MathLib\Types\Triangle.h"
#include "MathLib\Types\Tetrahedron.h"
#include "MathLib\Types\Sphere.h"
#include "MathLib\Types\Hypersphere.h"

// Custom Functions

#include "MathLib\Constants.h"

#include "MathLib\Functions\Core.h"
#include "MathLib\Functions\Matrices.h"

#include "MathLib\Functions\Collision2.h"
#include "MathLib\Functions\Collision3.h"

#include "MathLib\Functions\Noise.h"

// SDFs

#include "MathLib\SignedDistanceFields\SignedDistanceField.h"
#include "MathLib\SignedDistanceFields\SignedDistanceFieldAlterations.h"
#include "MathLib\SignedDistanceFields\SignedDistanceFieldBooleans.h"
#include "MathLib\SignedDistanceFields\SignedDistanceFieldPrimitives.h"
#include "MathLib\SignedDistanceFields\SignedDistanceFieldTransformations.h"
#include "MathLib\SignedDistanceFields\SignedDistanceFieldTypes.h"