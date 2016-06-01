in vec3 vecPoints;
in vec3 vecNormals;
in vec4 vecColors;

out vec4 color;
void main() {
	
	vec4 clippedVertex = vec4(vecPoints, 1.0);
	vec4 clippedNormal = vec4(vecNormals, 1.0);
	color = vecColors;
	
    
	vec4 finalVertex = matMVP * clippedVertex;
		
	gl_Position = finalVertex;
}