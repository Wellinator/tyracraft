#include "chunck.hpp"

Chunck::Chunck(Engine *t_engine, float offsetX, float offsetY, float offsetZ)
{
    engine = t_engine;
    texRepo = t_engine->renderer->getTextureRepository();
    meshes = new Mesh *[CHUNCK_SIZE * CHUNCK_SIZE];

    int mashIndex = 0;
    for (int i = 0; i < CHUNCK_SIZE; i++)
    {
        for (int j = 0; j < 1; j++)
        {
            for (int k = 0; k < CHUNCK_SIZE; k++)
            {
                float xPosition = offsetX + i * 14.0F;
                float yPosition = offsetY + j * 14.0F;
                float zPosition = offsetZ + k * 14.0F;

                blocks[mashIndex] = new Block(texRepo, xPosition, yPosition, zPosition);
                meshes[mashIndex] = &blocks[mashIndex]->mesh;
                mashIndex++;
            }
        }
    }
}

void Chunck::update(const Pad &t_pad, Camera &camera)
{
    for (int i = 0; i < CHUNCK_SIZE * CHUNCK_SIZE; i++)
    {
        // TODO -> Implemente occusion culling;
        blocks[i]->update(t_pad, camera);
        camera.update(engine->pad, blocks[i]->mesh);
    }
    engine->renderer->draw(meshes, CHUNCK_SIZE * CHUNCK_SIZE  );
};
