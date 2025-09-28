#include "transform.h"

Transform::Transform(){
	offsetX = 0.0f;
	offsetY = 0.0f;
	offsetZ = -1.0f;
	thetaX = 0.0f;
	thetaY = 0.0f;
	thetaZ = 0.0f;
	scale_factor = 2.0f;

	sens.translation = OFFSET;
	sens.rotation = ROTATION;
	sens.scale = SCALE;
}

void Transform::translate(float x, float y, float z){
	offsetX += x * sens.translation;
	offsetY += y * sens.translation;
	offsetZ += z * sens.translation;
}

void Transform::rotate(float x, float y, float z){
	thetaX += x * sens.rotation;
	thetaY += y * sens.rotation;
	thetaZ += z * sens.rotation;
}

void Transform::scale(float factor){
	scale_factor += factor * sens.scale;
}

glm::mat4 Transform::matrix(){
	glm::mat4x4 translation_matrix = glm::mat4x4(
		glm::vec4(1.0f,     0.0f,       0.0f, 0.0f),
		glm::vec4(0.0f,     1.0f,       0.0f, 0.0f),
		glm::vec4(0.0f,     0.0f,       1.0f, 0.0f),
		glm::vec4(offsetX,  offsetY,    offsetZ, 1.0f)
	);

	glm::mat4x4 rotation_matrix_z = glm::mat4x4(
		glm::vec4(cos(thetaZ), -sin(thetaZ), 0.0f, 0.0f),
		glm::vec4(sin(thetaZ), cos(thetaZ), 0.0f, 0.0f),
		glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
		glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
	);
	glm::mat4x4 rotation_matrix_y = glm::mat4x4(
		glm::vec4(cos(thetaY), 0.0f, sin(thetaY), 0.0f),
		glm::vec4(0.0f, 1.0f, 0.0f, 0.0f),
	glm::vec4(-sin(thetaY), 0.0f, cos(thetaY), 0.0f),
		glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
	);
	glm::mat4x4 rotation_matrix_x = glm::mat4x4(
		glm::vec4(1.0f, 0.0f, 0.0f, 0.0f),
		glm::vec4(0.0f, cos(thetaX), -sin(thetaX), 0.0f),
		glm::vec4(0.0f, sin(thetaX), cos(thetaX), 0.0f),
		glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
	);

	glm::mat4 scale_matrix = glm::mat4(
		glm::vec4(scale_factor, 0.0f, 0.0f, 0.0f),
		glm::vec4(0.0f, scale_factor, 0.0f, 0.0f),
		glm::vec4(0.0f, 0.0f, scale_factor, 0.0f),
		glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
	);

	glm::mat4 transformation = translation_matrix * scale_matrix * rotation_matrix_z * rotation_matrix_y * rotation_matrix_x;
	return transformation;
}
