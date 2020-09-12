#include "server.h"
//#define EMBED_SITE
void embed(HttpServer &server){
#ifdef EMBED_SITE
#include "embed_site.hpp"
#endif
}
