
#version 400

in vec3 vertexPosition;
uniform mat4 modelMatrix;
uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;

void main() {
  gl_Position = projectionMatrix * viewMatrix * modelMatrix * 
                vec4(vertexPosition, 1);
}
