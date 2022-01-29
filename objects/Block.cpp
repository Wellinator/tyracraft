#include "Block.hpp"
#include <models/mesh.hpp>
#include <loaders/obj_loader.hpp>
#include <utils/debug.hpp>

Block::Block(TextureRepository *t_texRepo, float X, float Y, float Z)
{
    texRepo = t_texRepo;
    speed = 1.000001F;

    mesh.loadObj("meshes/cube/", "cube", 7.0F, true);
    mesh.shouldBeFrustumCulled = true;
    mesh.shouldBeBackfaceCulled = true;
    mesh.shouldBeLighted = false;
    mesh.position.set(X, Y, Z);
    mesh.rotation.x = 0; //-1.566F;
    mesh.rotation.z = 0; //1.566F;

    texRepo->addByMesh("meshes/cube/", mesh, PNG);
    PRINT_LOG("Block created");
}

void Block::update(const Pad &t_pad, const Camera &t_camera)
{
    rotate(t_pad);
    Vector3 *nextPos = getNextPosition(t_pad, t_camera);
}

Vector3 *Block::getNextPosition(const Pad &t_pad, const Camera &t_camera)
{
    Vector3 *result = new Vector3(mesh.position);
    Vector3 normalizedCamera = Vector3(t_camera.unitCirclePosition);
    normalizedCamera.normalize();
    normalizedCamera *= speed;
    if (t_pad.lJoyV <= 100)
    {
        result->x += -normalizedCamera.x;
        result->z += -normalizedCamera.z;
    }
    else if (t_pad.lJoyV >= 200)
    {
        result->x += normalizedCamera.x;
        result->z += normalizedCamera.z;
    }
    if (t_pad.lJoyH <= 100)
    {
        result->x += -normalizedCamera.z;
        result->z += normalizedCamera.x;
    }
    else if (t_pad.lJoyH >= 200)
    {
        result->x += normalizedCamera.z;
        result->z += -normalizedCamera.x;
    }
    return result;
}

void Block::move(Pad &t_pad)
{
    if (t_pad.isDpadUpPressed)
    {
        return;
    }
}

void Block::rotate(const Pad &t_pad)
{
    if (t_pad.rJoyH >= 200)
    {
        PRINT_LOG("Rotating rJoyH >= 200");
        mesh.rotation.x -= speed;
        mesh.rotation.y -= speed;
        mesh.rotation.z -= speed;
    }
    else if (t_pad.rJoyH <= 100)
    {
        PRINT_LOG("Rotating rJoyH <= 100");
        mesh.rotation.x += speed;
        mesh.rotation.y += speed;
        mesh.rotation.z += speed;
    }
}