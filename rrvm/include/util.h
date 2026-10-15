#pragma once

#include <arpa/inet.h>

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define betohs(x) htons(x)
#define betohl(x) htonl(x)
#define letohs(x) x
#define letohl(x) x

#define htobes(x) htons(x)
#define htobel(x) htonl(x)
#define htoles(x) x
#define htolel(x) x
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define letohs(x) htons(x)
#define letohl(x) htonl(x)
#define betohs(x) x
#define betohl(x) x

#define htoles(x) htons(x)
#define htolel(x) htonl(x)
#define htobes(x) x
#define htobel(x) x
#endif
