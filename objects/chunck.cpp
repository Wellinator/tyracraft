#include "chunck.hpp"

Chunck::Chunck(Engine *t_engine, float offsetX, float offsetY, float offsetZ)
{
    engine = t_engine;
    texRepo = t_engine->renderer->getTextureRepository();
    meshes = new Mesh *[getMeshesCount()];

    int mashIndex = 0;
    for (int i = 0; i < CHUNCK_SIZE; i++)
    {
        for (int j = 0; j < CHUNCK_SIZE; j++)
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

void Chunck::update(Pad &t_pad, Camera &camera)
{
    // for (int i = 0; i < getMeshesCount(); i++)
    // {
    //     camera.update(t_pad, blocks[i]->mesh);
    // }
};
