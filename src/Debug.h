#ifndef DEBUG_H
#define DEBUG_H

#if DO_DEBUG == 1
    #define DEBUG(X) X
#else
    #define DEBUG(X)
#endif

#include <RendererContainer.h>

class Debug {
public:
    static void processDebugInput(const RendererContainer* renderers);
};


#endif //DEBUG_H
