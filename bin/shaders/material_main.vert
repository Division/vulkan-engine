#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(std140, set = 0, binding = 0) uniform Camera {
    vec3 cameraPosition;
    ivec2 cameraScreenSize;
    mat4 cameraViewMatrix;
    mat4 cameraProjectionMatrix;
} camera;

layout(std140, set = 0, binding = 1) uniform ObjectParams {
    mat4 objectModelMatrix;
    mat4 objectNormalMatrix;
    vec2 uvScale;
    vec2 uvOffset;
    uint layer;
} object_params;


layout(location = 0) in vec3 position;

#if defined(TEXTURE0)
layout(location = 2) in vec2 texcoord0;
layout(location = 1) out vec2 fragTexCoord;
#endif

#if defined(SKINNING)
layout(location = 4) in vec3 joint_weights;
layout(location = 3) in vec3 joint_indices;

layout (std140, set = 0, binding = 4) uniform SkinningMatrices {
  mat4 matrices[70];
} skinning;
#endif

layout(location = 0) out vec3 fragColor;

void main() {
    fragColor = vec3(1.0, 1.0, 1.0);

    mat4 model_matrix = object_params.objectModelMatrix;

	#if defined(SKINNING)
	model_matrix = mat4(0); // object model transform is included into bone matrices so just blend them
	for (int i = 0; i < 3; i++) {
		int joint_index = int(joint_indices[i]);
		float joint_weight = joint_weights[i];
		model_matrix += skinning.matrices[joint_index] * joint_weight;
	}
	#endif

    gl_Position = camera.cameraProjectionMatrix * camera.cameraViewMatrix * model_matrix * vec4(position, 1.0);

#if defined(TEXTURE0)
    fragTexCoord = texcoord0;
#endif
}
