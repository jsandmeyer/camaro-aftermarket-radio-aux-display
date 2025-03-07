#ifndef RENDERER_CONTAINER_H
#define RENDERER_CONTAINER_H

#include "Renderer.h"

class RendererContainer {
    /**
     * List of Renderer pointers
     */
    Renderer **renderers;

    /**
     * Number of Renderer pointers
     */
    unsigned int rendererCount;

public:
    /**
     * Create a RendererContainer
     * Must be instantiated with *exact* count of renderers
     * @param rendererCount number of renderers this list will contain
     */
    explicit RendererContainer(unsigned int rendererCount);

    /**
     * Places a renderer at a specific index (priority)
     * @param index the position to insert into (if >= rendererCount, method returns false)
     * @param renderer the Renderer to insert
     * @return whether the element could be inserted
     */
    bool setRenderer(size_t index, Renderer *renderer) const;

    /**
     * @return first element in the list
     */
    [[nodiscard]] Renderer **begin() const;

    /**
     * @return the last element in the list (nullptr)
     */
    [[nodiscard]] Renderer **end() const;

    /**
     * @param index the index to select
     * @return the element from the list (or nullptr)
     */
    Renderer *operator[] (size_t index) const;
};

#endif //RENDERER_CONTAINER_H
