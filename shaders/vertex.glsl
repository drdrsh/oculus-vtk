in vec3 vecPoints;
in vec3 vecNormals;
in uint vecSelection;

out Vertex vertex_cameraSpace;

flat out int  intersects;
flat out int  selected;


void main() {
	
	intersects = INTERSECT_NONE;
	selected = 0;
	vec4 clippedVertex = vec4(vecPoints, 1.0);
	vec4 clippedNormal = vec4(vecNormals, 1.0);
    
	vec4 finalVertex = matWVP * clippedVertex;
	
	vertex_cameraSpace.position = vec3( matObject * clippedVertex);
	vertex_cameraSpace.normal = normalize(matNormal * clippedNormal).xyz;
	 
	 
	if(vecSelection  > 0u){
		intersects = INTERSECT_SELECTED;
		selected = 1;
	}
	
	gl_Position = finalVertex;
}