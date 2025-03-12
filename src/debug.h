#ifndef DEBUG_H
#define DEBUG_H

#if DO_DEBUG == true
    #define DEBUG(X) X;
#else
    #define DEBUG(X)
#endif

#endif //DEBUG_H
