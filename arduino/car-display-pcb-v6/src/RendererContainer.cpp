#include "RendererContainer.h"

RendererContainer::RendererContainer(unsigned int const rendererCount) : rendererCount(rendererCount) {
    renderers = new Renderer*[rendererCount];
}

// ReSharper disable once CppMemberFunctionMayBeConst : actually edits state
void RendererContainer::setRenderer(size_t const index, Renderer* renderer) {
    renderers[index] = renderer;
    auto name = renderer->getName();
    Serial.printf("Renderer name %s\n", name);
}

Renderer **RendererContainer::begin() const {
    return &renderers[0];
}

Renderer **RendererContainer::end() const {
    return &renderers[rendererCount];
}

Renderer *RendererContainer::operator[] (size_t const index) const {
    if (index >= rendererCount) {
        return nullptr;
    }

    return renderers[index];
}
