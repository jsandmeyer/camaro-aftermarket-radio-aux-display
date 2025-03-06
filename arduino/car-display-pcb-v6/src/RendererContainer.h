#ifndef RENDERER_CONTAINER_H
#define RENDERER_CONTAINER_H

#include "Renderer.h"

class RendererContainer {
    Renderer **renderers;
    unsigned int rendererCount;
public:
    explicit RendererContainer(unsigned int rendererCount);
    void setRenderer(size_t index, Renderer *renderer);
    [[nodiscard]] Renderer **begin() const;
    [[nodiscard]] Renderer **end() const;
    Renderer *operator[] (size_t index) const;
};

#endif //RENDERER_CONTAINER_H
