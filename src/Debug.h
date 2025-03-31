#ifndef DEBUG_H
#define DEBUG_H

#if DO_DEBUG == 1
    #define DEBUG(X) X
#else
    #define DEBUG(X)
#endif

#include <Renderer.h>

class Debug {
public:
    static void processDebugInput(Renderer** renderers, size_t numRenderers);
};


#endif //DEBUG_H
