#include <models/mesh.hpp>
#include <loaders/obj_loader.hpp>
#include <utils/debug.hpp>

#include "chunck.hpp"

Chunck::Chunck(TextureRepository *t_texRepo, float offsetX, float offsetY, float offsetZ)
{
    meshes = new Mesh *[CHUNCK_SIZE * CHUNCK_SIZE * CHUNCK_SIZE];
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
    baseBlobk->mesh.loadObj("meshes/cube/", "cube", BOCK_SIZE, false);
    baseBlobk->mesh.position.set(0, 0, 0);
    texRepo->addByMesh("meshes/cube/", baseBlobk->mesh, PNG);

    blockIndex = 0;
    for (size_t x = 0; x < CHUNCK_SIZE; x++)
    {
        for (size_t y = 0; y < CHUNCK_SIZE; y++)
        {
            for (size_t z = 0; z < CHUNCK_SIZE; z++)
            {
                blocks[blockIndex].init(
                    baseBlobk->mesh,
                    x * BOCK_SIZE * 2,
                    y * -(BOCK_SIZE * 2),
                    z * -(BOCK_SIZE * 2));
                blocks[blockIndex].isHidden = !(
                    x == 0 or x == (CHUNCK_SIZE - 1) or
                    y == 0 or y == (CHUNCK_SIZE - 1) or
                    z == 0 or z == (CHUNCK_SIZE - 1));

                texRepo->getBySpriteOrMesh(baseBlobk->mesh.getMaterial(0).getId())->addLink(blocks[blockIndex].mesh.getMaterial(0).getId());
                meshes[blockIndex] = &blocks[blockIndex].mesh;
                blockIndex++;
            }
        }
    }
    PRINT_LOG("Chunck initialized!");
};

void Chunck::update(Engine *t_engine)
{
    for (size_t i = 0; i < (CHUNCK_SIZE * CHUNCK_SIZE * CHUNCK_SIZE); i++)
    {
        if (blocks[i].shouldBeDrawn())
            t_engine->renderer->draw(blocks[i].mesh);
    }
};
