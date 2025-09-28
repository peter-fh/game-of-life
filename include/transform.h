#include "glm/glm.hpp"

#define OFFSET 0.02f
#define ROTATION 0.05f
#define SCALE 0.05f


typedef struct {
    float translation;
    float rotation;
    float scale;
} Sensitivity;


class Transform {
public:
    Transform();
    Sensitivity sens;
    glm::mat4 matrix();

    void translate(float x, float y, float z);
    void rotate(float x, float y, float z);
    void scale(float factor);

private:
    float offsetX, offsetY, offsetZ;
    float thetaX, thetaY, thetaZ;
    float scale_factor;
};
