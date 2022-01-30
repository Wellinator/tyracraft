#include <models/mesh.hpp>
#include <loaders/obj_loader.hpp>
#include <utils/debug.hpp>

#include "chunck.hpp"

Chunck::Chunck(TextureRepository *t_texRepo, float offsetX, float offsetY, float offsetZ)
{
    meshes = new Mesh *[CHUNCK_SIZE * CHUNCK_SIZE];
    texRepo = t_texRepo;
    baseBlobk = new Block();
    initChunck();
};

Chunck::~Chunck()
{
}

void Chunck::initChunck()
{
    PRINT_LOG("Initializing chunck");
    PRINT_LOG("Creating base block");
    baseBlobk->mesh.loadObj("meshes/cube/", "cube", BOCK_SIZE, true);
    baseBlobk->mesh.position.set(0, 0, 0);
    texRepo->addByMesh("meshes/cube/", baseBlobk->mesh, PNG);
    baseBlobk->mesh.shouldBeFrustumCulled = true;
    baseBlobk->mesh.shouldBeBackfaceCulled = true;
    baseBlobk->mesh.shouldBeLighted = false;

    blockIndex = 0;
    for (size_t i = 0; i < CHUNCK_SIZE; i++)
    {
        for (size_t j = 0; j < CHUNCK_SIZE; j++)
        {
            blocks[blockIndex].init(baseBlobk->mesh, i * BOCK_SIZE * 2, 1, j * -(BOCK_SIZE * 2));
            texRepo->getBySpriteOrMesh(baseBlobk->mesh.getMaterial(0).getId())->addLink(blocks[blockIndex].mesh.getMaterial(0).getId());
            meshes[blockIndex] = &blocks[blockIndex].mesh;
            blockIndex++;
        }
    }
    PRINT_LOG("Chunck initialized!");
};

void Chunck::update(Engine *t_engine)
{
    for (size_t i = 0; i < (CHUNCK_SIZE * CHUNCK_SIZE); i++)
    {
        t_engine->renderer->draw(blocks[i].mesh);
    }
};
