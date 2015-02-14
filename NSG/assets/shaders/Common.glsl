//Remember to rebuild with CMake if this file changes
#ifdef GLES2
	#ifdef GL_FRAGMENT_PRECISION_HIGH
		precision highp float;
		precision highp int;
	#else
		precision mediump float;
		precision mediump int;
	#endif
#else
	#define lowp
	#define mediump
	#define highp
#endif   

varying vec4 v_color;
varying vec2 v_texcoord0; 

#ifdef HAS_LIGHTS
	varying vec3 v_normal;
	varying vec3 v_tangent;
	varying vec3 v_bitangent;
	varying vec2 v_texcoord1;
	varying vec3 v_vertexToEye;
	varying vec3 v_vertexToEyeInTangentSpace;

	struct Attenuation
	{
	    float constant;
	    float linear;
	    float quadratic;
	};

	struct BaseLight
	{
	    vec4 ambient;
	    vec4 diffuse;
	    vec4 specular;
	};

	uniform vec4 u_sceneAmbientColor;
	uniform vec3 u_eyeWorldPos;

	#ifdef HAS_DIRECTIONAL_LIGHTS
		struct DirectionalLight
		{
		    int enabled;
		    BaseLight base;
		    vec3 direction;
		};
		uniform DirectionalLight u_directionalLight[NUM_DIRECTIONAL_LIGHTS];
	#endif

	#ifdef HAS_POINT_LIGHTS
		varying vec3 v_lightDirection[NUM_POINT_LIGHTS];
		struct PointLight
		{
		    int enabled;
		    BaseLight base;
		    vec3 position;
		    Attenuation atten;
		};
		uniform PointLight u_pointLights[NUM_POINT_LIGHTS];
	#endif

	#ifdef HAS_SPOT_LIGHTS
		varying vec3 v_light2Pixel[NUM_SPOT_LIGHTS];
		struct SpotLight
		{
		    int enabled;
		    BaseLight base;
		    vec3 position;
		    vec3 direction;
		    Attenuation atten;
		    float cutOff; // 0.5f * cosine(cutOff)
		};
		uniform SpotLight u_spotLights[NUM_SPOT_LIGHTS];
	#endif

#elif defined(LIGHTMAP)
	varying vec2 v_texcoord1;
#endif

struct Material
{
	vec4 color;
#ifdef HAS_LIGHTS	
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    float shininess;
    float parallaxScale;
#endif    
};

uniform Material u_material;

