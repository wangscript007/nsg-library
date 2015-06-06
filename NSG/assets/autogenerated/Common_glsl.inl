#pragma once
namespace NSG
{
static const char* COMMON_GLSL = \
"//Remember to rebuild with CMake if this file changes\n"\
"#ifdef GLES2\n"\
"	#ifdef GL_FRAGMENT_PRECISION_HIGH\n"\
"		precision highp float;\n"\
"		precision highp int;\n"\
"	#else\n"\
"		precision mediump float;\n"\
"		precision mediump int;\n"\
"	#endif\n"\
"#else\n"\
"	#define lowp\n"\
"	#define mediump\n"\
"	#define highp\n"\
"#endif   \n"\
"#if defined(AMBIENT_PASS)\n"\
"	uniform vec4 u_sceneAmbientColor;\n"\
"	varying vec4 v_color;\n"\
"	varying vec2 v_texcoord0;\n"\
"	#if defined(AOMAP1) || defined(LIGHTMAP1)\n"\
"		varying vec2 v_texcoord1;\n"\
"	#endif\n"\
"#elif defined(SHADOWCUBE_PASS) || defined(SHADOW_PASS)\n"\
"	varying vec3 v_worldPos;\n"\
"#else // LIT_PASS\n"\
"	varying vec4 v_color;\n"\
"	varying vec2 v_texcoord0;\n"\
"	varying vec3 v_worldPos;\n"\
"	varying vec3 v_normal;\n"\
"	varying vec3 v_tangent;\n"\
"	varying vec3 v_bitangent;\n"\
"	varying vec2 v_texcoord1;\n"\
"	varying vec3 v_vertexToEye;\n"\
"	#if defined(SHADOWMAP)\n"\
"		varying vec4 v_shadowClipPos;\n"\
"		uniform float u_shadowMapInvSize;\n"\
"	#elif defined(CUBESHADOWMAP)\n"\
"		uniform float u_shadowMapInvSize;\n"\
"	#endif\n"\
"	struct BaseLight\n"\
"	{\n"\
"	    vec4 diffuse;\n"\
"	    vec4 specular;\n"\
"	};\n"\
"	#if defined(HAS_DIRECTIONAL_LIGHT)\n"\
"		uniform vec4 u_shadowColor;\n"\
"		varying vec3 v_lightDirection;\n"\
"		struct DirectionalLight\n"\
"		{\n"\
"		    BaseLight base;\n"\
"		    vec3 direction;\n"\
"		    vec3 position; // really is the shadow camera position\n"\
"		};\n"\
"		uniform DirectionalLight u_directionalLight;\n"\
"	#elif defined(HAS_POINT_LIGHT)\n"\
"		uniform vec4 u_shadowColor;\n"\
"		varying vec3 v_lightDirection;\n"\
"		struct PointLight\n"\
"		{\n"\
"		    BaseLight base;\n"\
"		    vec3 position;\n"\
"		};\n"\
"		uniform PointLight u_pointLight;\n"\
"	#elif defined(HAS_SPOT_LIGHT)\n"\
"		uniform vec4 u_shadowColor;\n"\
"		varying vec3 v_lightDirection;\n"\
"		struct SpotLight\n"\
"		{\n"\
"		    BaseLight base;\n"\
"		    vec3 position;\n"\
"		    vec3 direction;\n"\
"		    float cutOff; // 0.5f * cosine(cutOff)\n"\
"		};\n"\
"		uniform SpotLight u_spotLight;\n"\
"	#endif\n"\
"	uniform float u_shadowBias;\n"\
"#endif\n"\
"struct Material\n"\
"{\n"\
"	vec4 color;\n"\
"    float ambient;\n"\
"    vec4 diffuse;\n"\
"    vec4 specular;\n"\
"    float shininess;\n"\
"};\n"\
"uniform Material u_material;\n"\
"uniform vec3 u_eyeWorldPos;\n"\
"uniform float u_lightInvRange;\n"\
;
}
