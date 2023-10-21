#pragma once
#define STRINGIFY(...) #__VA_ARGS__
#define STR(...) STRINGIFY(__VA_ARGS__)


namespace ORNG {
	class CodedAssets { // Contains resources that need to be hard-coded into the library.
	public:

		static void Init() {
		}

		inline static const char* GBufferVS = {
			#include "../res/shaders/GBufferVS.glsl"
		};

		inline static const char* BloomDownsampleCS = {
			#include "../res/shaders/BloomDownsampleCS.glsl"
		};

		inline static const char* BloomThresholdCS = {
			#include "../res/shaders/BloomThresholdCS.glsl"
		};

		inline static const char* ColorFS = {
			#include "../res/shaders/ColorFS.glsl"
		};

		inline static const char* BloomUpsampleCS = {
			#include "../res/shaders/BloomUpsampleCS.glsl"
		};

		inline static const char* BlurFS = {
			#include "../res/shaders/BlurFS.glsl"
		};

		inline static const char* BRDFConvolutionFS = {
			#include "../res/shaders/BRDFConvolutionFS.glsl"
		};


		inline static const char* CubemapShadowFS = {
		#include "../res/shaders/CubemapShadowFS.glsl"
		};

		inline static const char* CubemapShadowVS = {
		#include "../res/shaders/CubemapShadowVS.glsl"
		};

		inline static const char* CubemapVS = {
		#include "../res/shaders/CubemapVS.glsl"
		};

		inline static const char* DepthFS = {
		#include "../res/shaders/DepthFS.glsl"
		};

		inline static const char* DepthVS = {
		#include "../res/shaders/DepthVS.glsl"
		};

		inline static const char* FogCS = {
		#include "../res/shaders/FogCS.glsl"
		};

		inline static const char* GBufferFS = {
		#include "../res/shaders/GBufferFS.glsl"
		};


		inline static const char* HDR_ToCubemapFS = {
		#include "../res/shaders/HDR_ToCubemapFS.glsl"
		};

		inline static const char* LightingCS = {
		#include "../res/shaders/LightingCS.glsl"
		};

		inline static const char* PostProcessCS = {
		#include "../res/shaders/PostProcessCS.glsl"
		};

		inline static const char* PortalCS = {
		#include "../res/shaders/PortalCS.glsl"
		};

		inline static const char* QuadFS = {
		#include "../res/shaders/QuadFS.glsl"
		};

		inline static const char* QuadVS = {
		#include "../res/shaders/QuadVS.glsl"
		};

		inline static const char* ReflectionFS = {
		#include "../res/shaders/ReflectionFS.glsl"
		};

		inline static const char* ReflectionVS = {
		#include "../res/shaders/ReflectionVS.glsl"
		};

		inline static const char* SkyboxDiffusePrefilterFS = {
		#include "../res/shaders/SkyboxDiffusePrefilterFS.glsl"
		};


		inline static const char* SkyboxSpecularPrefilterFS = {
		#include "../res/shaders/SkyboxSpecularPrefilterFS.glsl"
		};


		inline static const char* TransformVS = {
		#include "../res/shaders/TransformVS.glsl"
		};
	};
}
