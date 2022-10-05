#pragma once

#include "Rendering/CUDATypes.h"
#include "MathLib/Types/Ray.h"
#include "MathLib/Functions/Core.h"

enum class ProjectionMethod : bool
{
	Parallel	= 0,
	Perspectve	= 1
};

static const char* s_ProjectionMethodNames[(int) 2] = {
	"Parallel",
	"Projection"
};

template <typename VectorType>
struct A_CPUGPU_ALIGN(256) Camera
{
	// TODO: Upon understanding, why glm's <VectorType>::length() call is only constexpr for the VS compiler, but not for the CUDSA compiler, make this adapt to VectorType::length(). #CUDA_VS_VISUALSTUDIO
	using ToWorldSpaceMatrix_t = glm::mat<4, 4, float, glm::defaultp>; 

	Camera() = default;

	// Parameters for construction
	float FieldOfViewVerticalRadians;
	float ViewConeRadians;
	float AspectRatio;
	VectorType ForwardVector;
	VectorType RightVector;
	VectorType UpVector;
	VectorType OverVector;
	float ViewPaneDistance;
	
	float ViewPanePrimarySizeFactor		= 1.0f;
	float ViewPaneSecondarySizeFactor	= 1.0f;

	// Other parameters
	VectorType Position;

	ProjectionMethod PrimaryProjectionMethod	= ProjectionMethod::Perspectve;
	ProjectionMethod SecondaryProjectionMethod	= ProjectionMethod::Perspectve;

	// Calculated Values
	VectorType PrimaryViewPaneCenterOffset;
	VectorType SecondaryViewPaneCenterOffset;
	glm::vec2 ViewPaneHalfSize;

	// Cache
	float m_Last4DLatitude	= 0.0f;
	float m_Last3DLatitude	= 0.0f;
	float m_LastLongitude	= 0.0f;

	//////////////////////////////////////////////////////////////////////////

	void Initialize(const VectorType& position, const float fieldOfViewVerticalRadians, const float viewConeRadians, const float aspectRatio, 
		const VectorType& forwardVector, const VectorType& rightVector, const VectorType& upVector, const VectorType& overVector, 
		const float viewPaneDistance, const ProjectionMethod projectionPrimary, const ProjectionMethod projectionSecondary)
	{
		Position						= position; 
		FieldOfViewVerticalRadians		= fieldOfViewVerticalRadians;
		ViewConeRadians					= viewConeRadians;
		AspectRatio						= aspectRatio; 
		ForwardVector					= glm::normalize(forwardVector);
		RightVector						= glm::normalize(rightVector); 
		UpVector						= glm::normalize(upVector); 
		OverVector						= glm::normalize(overVector);
		ViewPaneDistance				= viewPaneDistance;
		PrimaryProjectionMethod			= projectionPrimary;
		SecondaryProjectionMethod		= projectionSecondary;
		ViewPaneHalfSize.y				= std::tan(fieldOfViewVerticalRadians / 2.0f) * viewPaneDistance;
		ViewPaneHalfSize.x				= ViewPaneHalfSize.y * aspectRatio;
		
		PrimaryViewPaneCenterOffset		= ForwardVector * ViewPaneDistance;
		SecondaryViewPaneCenterOffset	= OverVector * ViewPaneDistance;
	}

	//////////////////////////////////////////////////////////////////////////

	inline VectorType GetPosition() const
	{
		return Position;
	}

	//////////////////////////////////////////////////////////////////////////

	A_CUDA_CPUGPU inline VectorType GetOriginPosition(const float viewPercentage) const
	{
		// We do not curve the camera movement around the viewing pane, instead, we move along one axis alone
		const float offsetAngle	= (viewPercentage - 0.5f) * ViewConeRadians;
		const float offset		= ViewPaneDistance * std::tan(offsetAngle);

		return Position + RightVector * offset;
	}

	//////////////////////////////////////////////////////////////////////////

	A_CUDA_CPUGPU inline VectorType GetViewPaneTargetOffset(const float percentageX, const float percentageY) const
	{
		/*
			primaryViewPane       secondaryViewPane
				 
				  *------*          *------*
				  |  o   | dim2		| .-o  | dim2 
				  | /    |         .-'     |
				  *-/----*      .-' *------*
				   /'	dim1 .-'       dim1
				   /	  .-'
		rayDir1	  /'   .-'   rayDir2
				  /.-'
				origin
			   
			rayDir1	= local forward axis
			rayDir2	= local over axis
			dim1	= local right vector
			dim2	= local up vector
		*/

		const float offsetDim1				= (ViewPaneHalfSize.x * (2.0f * percentageX - 1.0f)); // is [-1, 1]
		const float offsetDim2				= (ViewPaneHalfSize.y * (2.0f * percentageY - 1.0f)); // is [-1, 1]
		const VectorType offsetVectorDim1	= RightVector * offsetDim1;
		const VectorType offsetVectorDim2	= UpVector * offsetDim2;

		return offsetVectorDim1 + offsetVectorDim2;
	}

	//////////////////////////////////////////////////////////////////////////

	A_CUDA_CPUGPU inline VectorType GetPrimaryViewPaneTargetPosition(float percentageX, float percentageY) const
	{
		const VectorType viewPaneCenter			= GetPrimaryViewPaneTargeCenter();
		const VectorType viewPaneTargetOffset	= ViewPanePrimarySizeFactor * GetViewPaneTargetOffset(percentageX, percentageY);

		return viewPaneCenter + viewPaneTargetOffset;
	}

	//////////////////////////////////////////////////////////////////////////

	A_CUDA_CPUGPU inline VectorType GetSecondaryViewPaneTargetPosition(float percentageX, float percentageY) const
	{
		const VectorType viewPaneCenter			= GetSecondaryViewPaneTargeCenter();		
		const VectorType viewPaneTargetOffset	= ViewPaneSecondarySizeFactor * GetViewPaneTargetOffset(percentageX, percentageY);

		return viewPaneCenter + viewPaneTargetOffset;
	}

	//////////////////////////////////////////////////////////////////////////

	A_CUDA_CPUGPU inline VectorType GetPrimaryViewPaneTargeCenter() const
	{
		return Position + PrimaryViewPaneCenterOffset;
	}

	//////////////////////////////////////////////////////////////////////////

	A_CUDA_CPUGPU inline VectorType GetSecondaryViewPaneTargeCenter() const
	{
		return Position + SecondaryViewPaneCenterOffset;
	}

	//////////////////////////////////////////////////////////////////////////

	A_CUDA_CPUGPU inline Math::BiRay<VectorType> GetBiray(float viewPercentage, float inViewPercentageX, float inViewPercentageY, ToWorldSpaceMatrix_t& outToWorldSpaceMatrix) const
	{
		VectorType birayOrigin				= GetOriginPosition(viewPercentage);
		VectorType birayMainDirection		= ForwardVector;	// Parallel projection as a default
		VectorType biraySecondaryDirection	= OverVector;		// Parallel projection as a default

		if (PrimaryProjectionMethod == ProjectionMethod::Perspectve)
		{
			const VectorType targetPosition	= GetPrimaryViewPaneTargetPosition(inViewPercentageX, inViewPercentageY);
			birayMainDirection = glm::normalize(targetPosition - birayOrigin);
		}
		else
		{
			birayOrigin	+= GetViewPaneTargetOffset(inViewPercentageX, inViewPercentageY);
		}

		if (SecondaryProjectionMethod == ProjectionMethod::Perspectve)
		{
			const VectorType targetPosition	= GetSecondaryViewPaneTargetPosition(inViewPercentageX, inViewPercentageY);
			biraySecondaryDirection	= glm::normalize(targetPosition - birayOrigin);
		}
		else
		{
			birayOrigin	+= GetViewPaneTargetOffset(inViewPercentageX, inViewPercentageY);
		}

		//////////////////////////////////////////////////////////////////////////
		// Make matrix

		if constexpr (std::is_same<VectorType, glm::vec4>::value)
		{
			//float matrix[16] = {
			//	RightVector.x, UpVector.x, birayMainDirection.x, biraySecondaryDirection.x,
			//	RightVector.y, UpVector.y, birayMainDirection.y, biraySecondaryDirection.y,
			//	RightVector.z, UpVector.z, birayMainDirection.z, biraySecondaryDirection.z,	
			//	RightVector.w, UpVector.w, birayMainDirection.w, biraySecondaryDirection.w,	
			//};

			// Column mayor
			float matrix[16] = {
				RightVector.x,				RightVector.y,				RightVector.z,				RightVector.w,
				UpVector.x,					UpVector.y,					UpVector.z,					UpVector.w,
				birayMainDirection.x,		birayMainDirection.y,		birayMainDirection.z,		birayMainDirection.w,	
				biraySecondaryDirection.x,	biraySecondaryDirection.y,	biraySecondaryDirection.z,	biraySecondaryDirection.w,	
			};

			outToWorldSpaceMatrix = glm::make_mat4x4(matrix);
		}

		//////////////////////////////////////////////////////////////////////////

		return Math::BiRay<VectorType>(birayOrigin, birayMainDirection, biraySecondaryDirection);
	}

	//////////////////////////////////////////////////////////////////////////

	inline void MoveLocal(VectorType localOffset) 
	{
		Position += localOffset;
	}

	//////////////////////////////////////////////////////////////////////////

	inline void Reset()
	{
		Position = VectorType();
	}

	//////////////////////////////////////////////////////////////////////////
	
	/*
	
	// We have 4 poles. 
	// Two are at +w/-w. [4D Latitude]
	// Two are at +z/-z. [3D Latitude]

	// Longitude is 360° [Once around the circle]
	// Both latitudes are only 180°, to not duplicate any position. If a value goes higher, a location at the opposite of the sphere is reached.
	// This would probably cause issues with the camera local vectors, so we stop it.

	// The 4D Latitude takes the 4D Hypersphere and "selects" which 3D sphere we are looking at.
	// The 3D Latitude takes the 3D Sphere and "selects" which 2D circle we are looking at.
	// The 2D Longitude takes the 2D Cirlce and "selects" which 1D point we are looking at.

	// Longitude
	// If the longitude is at an angle of 0° (which would be the equator, when compared to the earth), the sphere that is selected by the longitude has radius 0.
	// This is a pole at maximum w.

	const float PI			= glm::pi<float>();
	const float PI_HALF		= PI / 2.0f;
	const float PI_THIRD	= PI / 3.0f;
	const float PI_QUART	= PI / 4.0f;
	const float PI_EIGTH	= PI / 8.0f;

	Camera<DimensionVector>::UpdatePosition(512, 0, 0, 0);
	printf("#############################\n");
	printf("Rotate 4D Latitude [z->w]\n"); // 4D latitude (180°) /in the zw plane
	printf("#############################\n");
	Camera<DimensionVector>::UpdatePosition(512, 0 * PI_EIGTH, 0, 0);
	Camera<DimensionVector>::UpdatePosition(512, 1 * PI_EIGTH, 0, 0);
	Camera<DimensionVector>::UpdatePosition(512, 2 * PI_EIGTH, 0, 0);
	Camera<DimensionVector>::UpdatePosition(512, 3 * PI_EIGTH, 0, 0);
	Camera<DimensionVector>::UpdatePosition(512, 4 * PI_EIGTH, 0, 0);
	Camera<DimensionVector>::UpdatePosition(512, 5 * PI_EIGTH, 0, 0);
	Camera<DimensionVector>::UpdatePosition(512, 6 * PI_EIGTH, 0, 0);
	Camera<DimensionVector>::UpdatePosition(512, 7 * PI_EIGTH, 0, 0);
	
	printf("#############################\n");
	printf("Rotate 3D Latitude I [x->z]\n"); // 3D latitude (180°) in xz or yz plane
	printf("#############################\n");
	Camera<DimensionVector>::UpdatePosition(512, PI_HALF, 0 * PI_EIGTH, PI_HALF);
	Camera<DimensionVector>::UpdatePosition(512, PI_HALF, 1 * PI_EIGTH, PI_HALF);
	Camera<DimensionVector>::UpdatePosition(512, PI_HALF, 2 * PI_EIGTH, PI_HALF);
	Camera<DimensionVector>::UpdatePosition(512, PI_HALF, 3 * PI_EIGTH, PI_HALF);
	Camera<DimensionVector>::UpdatePosition(512, PI_HALF, 4 * PI_EIGTH, PI_HALF);
	Camera<DimensionVector>::UpdatePosition(512, PI_HALF, 5 * PI_EIGTH, PI_HALF);
	Camera<DimensionVector>::UpdatePosition(512, PI_HALF, 6 * PI_EIGTH, PI_HALF);
	Camera<DimensionVector>::UpdatePosition(512, PI_HALF, 7 * PI_EIGTH, PI_HALF);
	printf("#############################\n");
	printf("... AND [y->z]\n");
	printf("#############################\n");	
	Camera<DimensionVector>::UpdatePosition(512, PI_HALF, 0 * PI_EIGTH, 0);
	Camera<DimensionVector>::UpdatePosition(512, PI_HALF, 1 * PI_EIGTH, 0);
	Camera<DimensionVector>::UpdatePosition(512, PI_HALF, 2 * PI_EIGTH, 0);
	Camera<DimensionVector>::UpdatePosition(512, PI_HALF, 3 * PI_EIGTH, 0);
	Camera<DimensionVector>::UpdatePosition(512, PI_HALF, 4 * PI_EIGTH, 0);
	Camera<DimensionVector>::UpdatePosition(512, PI_HALF, 5 * PI_EIGTH, 0);
	Camera<DimensionVector>::UpdatePosition(512, PI_HALF, 6 * PI_EIGTH, 0);
	Camera<DimensionVector>::UpdatePosition(512, PI_HALF, 7 * PI_EIGTH, 0);
	
	printf("#############################\n");
	printf("Rotate Longitude [x->y] \n"); // 3D longitude (360°) up/right in xy plane
	printf("#############################\n");
	Camera<DimensionVector>::UpdatePosition(512, -2 * PI_THIRD, - PI_HALF, 0 * PI_QUART);
	Camera<DimensionVector>::UpdatePosition(512, -2 * PI_THIRD, - PI_HALF, 1 * PI_QUART);
	Camera<DimensionVector>::UpdatePosition(512, -2 * PI_THIRD, - PI_HALF, 2 * PI_QUART);
	Camera<DimensionVector>::UpdatePosition(512, -2 * PI_THIRD, - PI_HALF, 3 * PI_QUART);
	Camera<DimensionVector>::UpdatePosition(512, -2 * PI_THIRD, - PI_HALF, 4 * PI_QUART);
	Camera<DimensionVector>::UpdatePosition(512, -2 * PI_THIRD, - PI_HALF, 5 * PI_QUART);
	Camera<DimensionVector>::UpdatePosition(512, -2 * PI_THIRD, - PI_HALF, 6 * PI_QUART);
	Camera<DimensionVector>::UpdatePosition(512, -2 * PI_THIRD, - PI_HALF, 7 * PI_QUART);

	int n = 0;
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	std::cin >> n;
	printf("Value: %i", n);


	*/

	void UpdatePosition(const float latitude4D_ZW, const float latitude3D_YZ, const float longitude_XY)
	{
		// Links:
		// https://math.stackexchange.com/questions/56582/analogue-of-spherical-coordinates-in-n-dimensions
		// https://en.wikipedia.org/wiki/N-sphere#Spherical_coordinates
		// https://ef.gy/linear-algebra:normal-vectors-in-higher-dimensional-spaces
		// > https://math.stackexchange.com/questions/904172/how-to-find-a-4d-vector-perpendicular-to-3-other-4d-vectors/904177#904177 
		// https://i.stack.imgur.com/HIQvD.png
		// https://math.stackexchange.com/questions/239412/analytically-derive-n-spherical-coordinates-conversions-from-cartesian-coordinat
		// https://www.jstor.org/stable/2688595?read-now=1&seq=5#page_scan_tab_contents // this a good source!

		// Derivation of the new local vectors:
		// right:
		// > Take (1, 0, 0, 0) and rotate it by the longitude in the XY plane.
		// up:
		// > Take (0, 1, 0, 0) and rotate it by the 3D latitude in the YZ plane. Afterwards, rotate it by the longitude in the XY plane.
		// forward:
		// > The vector that points toward the origin
		// over:
		// > Use the ternary cross product of the other three vectors.
		
		if (latitude4D_ZW == m_Last4DLatitude && latitude3D_YZ == m_Last3DLatitude && longitude_XY == m_LastLongitude)
		{
			return;
		}

		m_Last4DLatitude = latitude4D_ZW;
		m_Last3DLatitude = latitude3D_YZ;
		m_LastLongitude  = longitude_XY;

		// Position
		Position		= Math::PolarToEuclidean({ViewPaneDistance, latitude4D_ZW, latitude3D_YZ, longitude_XY});

		const float DEGR = 0.0f;

		// Rotation
		RightVector		= Math::RotYZ(latitude3D_YZ) * Math::RotXY(longitude_XY) * glm::vec4(1, 0, 0, 0);
		UpVector		= Math::RotYZ(latitude3D_YZ) * glm::vec4(0, 1, 0, 0);
		ForwardVector	= glm::normalize(Math::RotZW(glm::radians(DEGR)) * -Position);	// < Always look at 0,0,0,0, but rotate a bit upwards in the zw plane.
		OverVector		= glm::normalize(Math::TernaryCross(RightVector, UpVector, ForwardVector));
		
		PrimaryViewPaneCenterOffset		= ForwardVector * ViewPaneDistance;
		SecondaryViewPaneCenterOffset	= OverVector * ViewPaneDistance;

		//printf("pol.ang: %.0f deg, %.0f deg, %.0f deg \n", glm::degrees(latitude4D_ZW), glm::degrees(latitude_3D_YZ), glm::degrees(longitude_XY));
		//printf("cartes.: %.1f, %.1f, %.1f, %.1f \n", newPosition.x, newPosition.y, newPosition.z, newPosition.w);
		//printf("local-x: %.1f, %.1f, %.1f, %.1f \n", right.x, right.y, right.z, right.w);
		//printf("local-y: %.1f, %.1f, %.1f, %.1f \n", up.x, up.y, up.z, up.w);
		//printf("local-z: %.1f, %.1f, %.1f, %.1f \n", forward.x, forward.y, forward.z, forward.w);
		//printf("local-w: %.1f, %.1f, %.1f, %.1f \n \n", over.x, over.y, over.z, over.w);
	}
};

