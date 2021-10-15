#version 400

uniform vec3 modelColor; 
uniform vec2 windowInfo;
out vec4 pixelColor; 

void main() { 

if( (windowInfo.y/2+1 > gl_FragCoord.y && gl_FragCoord.y > windowInfo.y/2-1 && gl_FragCoord.x < windowInfo.x/2+30 && gl_FragCoord.x > windowInfo.x/2-30)
   		|| (windowInfo.x/2+1 > gl_FragCoord.x && gl_FragCoord.x > windowInfo.x/2-1 && gl_FragCoord.y < windowInfo.y/2+30 && gl_FragCoord.y > windowInfo.y/2-30) )
	pixelColor = vec4(1 -  modelColor.x, 1 - modelColor.y, 1 - modelColor.z, 1);
else
   	pixelColor = vec4(modelColor, 1.0); 
  
} 
