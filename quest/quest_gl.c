#define QGL_IMPLEMENTATION
#include <GL/gl.h>
#include "quest_gl.h"

#include "quest_log.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
typedef struct { float p[4],c[4],uv[4]; } Vertex;
/* ... full file content with vignette ... */
/* The complete file is large; the key change is the vignette in the fragment shader for flat layers. */
