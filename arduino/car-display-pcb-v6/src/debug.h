#ifndef DEBUG_H
#define DEBUG_H

#ifdef DO_DEBUG
    #define DEBUG(X) X;
#else
    #define DEBUG(X)
#endif

#endif //DEBUG_H
