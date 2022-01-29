#include "Block.hpp"
#include <models/mesh.hpp>
#include <loaders/obj_loader.hpp>
#include <utils/debug.hpp>

Block::Block(TextureRepository *t_texRepo, float X, float Y, float Z)
{
    texRepo = t_texRepo;

    mesh.loadObj("meshes/cube/", "cube", 7.0F, true);
    mesh.shouldBeFrustumCulled = true;
    mesh.shouldBeBackfaceCulled = false;
    mesh.shouldBeLighted = false;
    mesh.position.set(X, Y, Z);
    mesh.rotation.x = -1.566F;
    mesh.rotation.z = 1.566F;

    texRepo->addByMesh("meshes/cube/", mesh, PNG);
    PRINT_LOG("Block created");
}