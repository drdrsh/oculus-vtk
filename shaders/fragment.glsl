
in      Vertex vertex_cameraSpace;
flat in int    selected;

int intersects;
out vec4 output_color;
bool intersectPlane(in Ray r, in Vertex v, out IntersectionData d) { 
		
	r.direction = normalize(r.direction);
	v.normal = normalize(v.normal);

    // assuming vectors are all normalized
    float denom = dot(r.direction, v.normal); 
    if (denom < 1e-15) { 
		return false;
	}
	 
	float numerator = dot( (v.position - r.origin), v.normal );
	//float numerator = dot( (r.origin - v.position), v.normal );
	
	d.distance_to_origin = numerator / denom;
	d.intersection_point = r.origin + (d.distance_to_origin *  r.direction);
	d.distance_to_intersection = length( d.intersection_point -  v.position );
		
	return true;
	
}

void processIntersections() {
			
	Ray r;
	Vertex v;
	IntersectionData d;	
	bool result;
	
	//Mouse look
	r = Ray(vecMouseStart, vecMouseEnd - vecMouseStart);
	v = vertex_cameraSpace;
	
	result = intersectPlane(r, v, d);
	
	if(result && d.distance_to_intersection < fSelectionSize){
		intersects = INTERSECT_MOUSE;
	}
	
	//Camera Look
	r = Ray(vecCameraPosition, (vecLookAt * 1000) - vecCameraPosition);
	v = vertex_cameraSpace;
	
	result = intersectPlane(r, v, d);
	if(result && d.distance_to_intersection < fSelectionSize){
		intersects = INTERSECT_EYE;
	}

}


void main (void) 
{  

	
	intersects = INTERSECT_NONE;
	if(selected == 0) {
		processIntersections();
	} else {
		intersects = INTERSECT_SELECTED;
	}

	if(intersects == INTERSECT_MOUSE) {
		switch(iSelectionMode) {
			case SELECT_MODE_SUBTRACT: output_color = vec4(0.0, 0.0, 0.0, 1.0); return;
			case SELECT_MODE_CLEAR:    output_color = vec4(1.0, 1.0, 1.0, 1.0); return;
			case SELECT_MODE_ADD:      output_color = vec4(0.0, 1.0, 1.0, 1.0); return;
		}
	} 

	if (intersects == INTERSECT_EYE) {
		switch(iSelectionMode) {
			case SELECT_MODE_SUBTRACT: output_color = vec4(1.0, 0.0, 1.0, 1.0); return;
			case SELECT_MODE_CLEAR:    output_color = vec4(1.0, 0.0, 0.0, 1.0); return;
			case SELECT_MODE_ADD:      output_color = vec4(1.0, 1.0, 0.0, 1.0); return;
		}
	}

	if (intersects == INTERSECT_SELECTED) {
		output_color = vec4(0.0, 1.0, 0.0, 1.0);
		return;
	}

	vec3 N = normalize(vertex_cameraSpace.normal);
	//vec3 v = vertex_cameraSpace.position;
	vec3 v = (gl_FragCoord * matView).xyz;

	vec4 finalColor = vec4(0.0, 0.0, 0.0, 0.0);
    
	Material selectedMaterial = BackMaterial;
	
	for (int i=0; i<MAX_LIGHTS; i++) {

		if(!Lights[i].enabled) {
			continue;
		}
		
	    vec4 p = vec4(Lights[i].position.xyz, 1.0) * matObject;
		//vec4 p = vec4(vecCameraPosition, 1.0);
		float distance = length(p.xyz - v);
		
		float lightPower = 35000.0;
		float d2 = distance * distance;
		
		vec3 L = normalize(p.xyz - v); 
		//vec3 E = normalize(vec3(0.0, 0.0, 0.0) - v);
		vec3 E = normalize(vecLookAt);
		vec3 R = normalize(reflect(-L, N)); 
   
		float cosAlpha = clamp(dot(E, R), 0, 1);
		float cosTheta = clamp(dot(N, L), 0, 1);
		
		//calculate Ambient Term:
		vec4 Iamb = 
			Lights[i].ambient 
			* selectedMaterial.ambient
			* 1/MAX_LIGHTS
			;

		//calculate Diffuse Term:
		vec4 Idiff = 
			lightPower 
			* Lights[i].diffuse 
			* selectedMaterial.diffuse 
			* cosTheta
			* 0.00001
			;

		//calculate Specular Term:
		vec4 Ispec = 
			lightPower
			* Lights[i].specular
			* selectedMaterial.specular
			* pow(cosAlpha, 15 * selectedMaterial.shininess)
			//* 1/d2
			;
	   
		finalColor += Iamb + Idiff + Ispec ;
	}
   
	if (intersects == INTERSECT_NONE) {
		output_color = finalColor; 
	}
	
}


