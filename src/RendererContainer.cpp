#include "RendererContainer.h"

/**
 * Create a RendererContainer
 * Must be instantiated with *exact* count of renderers
 * @param rendererCount number of renderers this list will contain
 */
RendererContainer::RendererContainer(uint16_t const rendererCount) : rendererCount(rendererCount) {
    renderers = new Renderer*[rendererCount + 1];
}

/**
 * Places a renderer at a specific index (priority)
 * @param index the position to insert into (if >= rendererCount, method returns false)
 * @param renderer the Renderer to insert
 * @return whether the element could be inserted
 */
bool RendererContainer::setRenderer(size_t const index, Renderer* renderer) const {
    if (index >= rendererCount) {
        return false;
    }

    renderers[index] = renderer;
    return true;
}

/**
 * @return first element in the list
 */
Renderer **RendererContainer::begin() const {
    return &renderers[0];
}

/**
 * @return the last element in the list (nullptr)
 */
Renderer **RendererContainer::end() const {
    return &renderers[rendererCount];
}

/**
 * @param index the index to select
 * @return the element from the list (or nullptr)
 */
Renderer *RendererContainer::operator[] (size_t const index) const {
    if (index >= rendererCount) {
        return nullptr;
    }

    return renderers[index];
}
