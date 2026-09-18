#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
//! There were warning which I did not like so they are silenced
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996)
#endif
#include "stb_image_write.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif
