#ifdef NDEBUG
#define STBI_NO_FAILURE_STRINGS
#else
#define STBI_FAILURE_USERMSG
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#define CGLTF_IMPLEMENTATION
#include <cgltf/cgltf.h>

#define FAST_OBJ_IMPLEMENTATION
#include <fast_obj/fast_obj.h>