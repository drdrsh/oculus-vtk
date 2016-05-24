#version 450

#define INTERSECT_NONE   	1
#define INTERSECT_MOUSE  	2
#define INTERSECT_EYE    	3
#define INTERSECT_SELECTED 	4

#define SELECT_MODE_CLEAR    0
#define SELECT_MODE_ADD      1
#define SELECT_MODE_SUBTRACT 2

#define MAX_LIGHTS 			7

struct LightSource {
	vec4 ambient;              // Aclarri   
	vec4 diffuse;              // Dcli   
	vec4 specular;             // Scli   
	vec3 position;             // Ppli   
	vec4 halfVector;           // Derived: Hi   
	vec3 spotDirection;        // Sdli   
	float spotExponent;        // Srli   
	float spotCutoff;          // Crli                              
	float spotCosCutoff;       // Derived: cos(Crli)                 
	float constantAttenuation; // K0   
	float linearAttenuation;   // K1   
	float quadraticAttenuation;// K2
	bool  enabled;
};

struct Material {
	vec4 emission;    // Ecm   
	vec4 ambient;     // Acm   
	vec4 diffuse;     // Dcm   
	vec4 specular;    // Scm   
	float shininess;  // Srm  
};

struct Ray {
  vec3 origin;
  vec3 direction; 
};

struct Vertex {
	vec3 position;
	vec3 normal;
};

struct IntersectionData {
	vec3  intersection_point;
	float distance_to_origin;
	float distance_to_intersection;
};

uniform mat4 matWVP; 
uniform mat4 matObject;
uniform mat4 matProjection;
uniform mat4 matView;
uniform mat4 matNormal;

uniform LightSource Lights[MAX_LIGHTS];
uniform Material    FrontMaterial;
uniform Material    BackMaterial;


uniform float fSelectionSize;
uniform int   iSelectionMode;
uniform vec3  vecCameraPosition;
uniform vec3  vecLookAt;
uniform vec3  vecMouseStart;
uniform vec3  vecMouseEnd;
