#pragma once

#include <MathLib\MathLib.h>
#include <type_traits>

namespace SDFFactory
{
	constexpr float SIZE	= 7.0f;
	constexpr float SPACE	= 17.0f;
	constexpr float SPAN	= 1.0f;

	// Needs to be inline as to stop multiple files including this .h file doing their own definition of this function each.
	// Q: Why is this even in the .h file? 
	// A: So that the using below can see our body, allowing it to use type deduction to propagate our return type unter a new name to other files
	auto inline CreateSDF_HyperCubeSpheres(/*void** transformPtr = nullptr*/)
	{	

		auto sdfVert						= Math::SDFHyperSphere(SIZE);
		auto sdfVertT						= Math::SDFTranslation<decltype(sdfVert)>(std::move(sdfVert), Math::Vector4(0, 0, 0, 0));
		auto sdfAllVerts					= Math::SDFFiniteRepetition<decltype(sdfVertT)>(std::move(sdfVertT), glm::vec4(SPACE, SPACE, SPACE, SPACE), glm::vec4(SPAN, SPAN, SPAN, SPAN));

		glm::mat4 vertsTransformation		= glm::identity<glm::mat4>();
		auto sdfVertsTransformation			= Math::SDFTransformation4x4<decltype(sdfAllVerts)>(std::move(sdfAllVerts), vertsTransformation);
		auto sdfVertsTranslation			= Math::SDFTranslation<decltype(sdfVertsTransformation)>(std::move(sdfVertsTransformation), glm::vec4());

		//////////////////////////////////////////////////////////////////////////

		// The new operator of SDFs is overwritten to be allocated by CUDA Managed	
		return new decltype(sdfVertsTranslation)(std::move(sdfVertsTranslation)); // < Move construct the sdf into our SDF Base pointer.
	}

	auto inline CreateSDF_HyperCube()
	{	

		auto sdfCube						= Math::SDFBox<glm::vec4>(SIZE * 2 * glm::vec4(1, 1, 1, 1));

		glm::mat4 vertsTransformation		= glm::identity<glm::mat4>();
		auto sdfVertsTransformation			= Math::SDFTransformation4x4<decltype(sdfCube)>(std::move(sdfCube), vertsTransformation);
		auto sdfVertsTranslation			= Math::SDFTranslation<decltype(sdfVertsTransformation)>(std::move(sdfVertsTransformation), glm::vec4(0, 0, 0, 0));

		auto sdfFloor						= Math::SDFBox<glm::vec4>(glm::vec4(40, 2, 400, 10));
		auto sdfFloorTranslation			= Math::SDFTranslation<decltype(sdfFloor)>(std::move(sdfFloor), glm::vec4(0, -30, 100, 0));

		auto sdfBoolean						= Math::SDFUnion<Math::SDFTranslation<Math::SDFTransformation4x4<Math::SDFBox<glm::vec4>>>, 
															 Math::SDFTranslation<Math::SDFBox<glm::vec4>>> 
															 (std::move(sdfVertsTranslation), std::move(sdfFloorTranslation));
		auto sdfBooleanTranslation			= Math::SDFTranslation<decltype(sdfBoolean)>(std::move(sdfBoolean), glm::vec4(0, 0, 0, 0));

		//////////////////////////////////////////////////////////////////////////

		// The new operator of SDFs is overwritten to be allocated by CUDA Managed	
		return new decltype(sdfBooleanTranslation)(std::move(sdfBooleanTranslation)); // < Move construct the sdf into our SDF Base pointer.
	}

	auto inline CreateSDF_HyperSphere()
	{	

		auto sdfCube						= Math::SDFHyperSphere(50.0f);
		
		glm::mat4 vertsTransformation		= glm::identity<glm::mat4>();
		auto sdfVertsTransformation			= Math::SDFTransformation4x4<decltype(sdfCube)>(std::move(sdfCube), vertsTransformation);
		auto sdfVertsTranslation			= Math::SDFTranslation<decltype(sdfVertsTransformation)>(std::move(sdfVertsTransformation), glm::vec4());

		//////////////////////////////////////////////////////////////////////////

		// The new operator of SDFs is overwritten to be allocated by CUDA Managed	
		return new decltype(sdfVertsTranslation)(std::move(sdfVertsTranslation)); // < Move construct the sdf into our SDF Base pointer.
	}

	// #SDFType: using SDF_HyperCubeSpheres_ptr_t = std::invoke_result<decltype(&SDFFactory::CreateSDF_HyperCubeSpheres)>::type;

	//////////////////////////////////////////////////////////////////////////

	auto inline CreateSDF_3DPlayGround()
	{
		auto sdfPrimitive1					= Math::SDFSphere(30.0f);
		auto sdfPrimitive1Translated		= Math::SDFTranslation<decltype(sdfPrimitive1)>(std::move(sdfPrimitive1), Math::Vector3(-30.0f, 00.0f, 20.0f));

		auto sdfPrimitive2					= Math::SDFBox<glm::vec3>(glm::vec3(20.0f, 20.0f, 20.0f));
		auto sdfPrimitive2Translated		= Math::SDFTranslation<decltype(sdfPrimitive2)>(std::move(sdfPrimitive2), Math::Vector3(00.0f, 00.0f, 00.0f));
		glm::mat4 transformation			= glm::rotate(glm::mat4(1.0), 90.0f / 360.0f * 6.28f, glm::vec3(1.0f, 1.0f, 1.0f));
		auto sdfPrimitive2Transformed		= Math::SDFTransformation4x4<decltype(sdfPrimitive2Translated)>(std::move(sdfPrimitive2Translated), transformation);

		auto sdfCombination					= Math::SDFSmoothUnion<decltype(sdfPrimitive2Transformed), decltype(sdfPrimitive1Translated)>(std::move(sdfPrimitive2Transformed), std::move(sdfPrimitive1Translated), 10.f);

		//////////////////////////////////////////////////////////////////////////

		return new decltype(sdfCombination)(std::move(sdfCombination)); // < Move construct the sdf into our SDF Base pointer.
	}

	// #SDFType: using SDF_3DPlayGround_ptr_t = std::invoke_result<decltype(&CreateSDF_3DPlayGround)>::type;	
}