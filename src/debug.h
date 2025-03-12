#ifndef DEBUG_H
#define DEBUG_H

#if DO_DEBUG == 1
    #define DEBUG(X) X;
#else
    #define DEBUG(X)
#endif

#endif //DEBUG_H
