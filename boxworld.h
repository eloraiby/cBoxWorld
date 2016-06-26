#ifndef BOXWORLD_H
#define BOXWORLD_H
/*
** BoxWorld Copyright 2016(c) Wael El Oraiby. All Rights Reserved
**
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** Under Section 7 of GPL version 3, you are granted additional
** permissions described in the GCC Runtime Library Exception, version
** 3.1, as published by the Free Software Foundation.
**
** You should have received a copy of the GNU General Public License and
** a copy of the GCC Runtime Library Exception along with this program;
** see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
** <http://www.gnu.org/licenses/>.
**
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <malloc.h>
#include <math.h>
#include "c99-3d-math/mathlib.h"
#include "emuGLES2/emuGLES2.h"

typedef enum {
	NO_ERROR		= 0,
	FILE_NOT_FOUND,
	INVALID_FORMAT,
	NOT_ENOUGH_MEMORY,
	UNSUPPORTED,
} BOXWORLD_ERROR;

extern void*			boxworld_error(BOXWORLD_ERROR err, const char* string);
extern BOXWORLD_ERROR	boxworld_error_number();
extern const char*		boxworld_error_string();


typedef enum {
	PF_A8,
	PF_R8G8B8,
	PF_R8G8B8A8
} PIXEL_FORMAT;

typedef struct {
	uint32			width;
	uint32			height;
	PIXEL_FORMAT	format;
	void*			pixels;
} image_t;

image_t*	image_allocate(int width, int height, PIXEL_FORMAT fmt);
void		image_release(image_t* img);
image_t*	image_load_png(const char* path);


/*
 *
 */
typedef struct {
	vec2_t		position;
	vec2_t		tex;
	color4_t	color;
} render_vertex_t;

typedef	render_vertex_t	render_quad_t[6];

enum {
	MAX_QUADS	= 8192,
};

typedef struct {
	GLuint	texture;
	GLuint	vbo;
	GLuint	program;

	GLuint	uniViewport;
	GLuint	uniTexture;

	GLuint	attrPosition;
	GLuint	attrTexCoord;
	GLuint	attrColor;

	render_quad_t	quads[MAX_QUADS];

	uint32	numQuads;
} gfx_context_t;

gfx_context_t*			renderer_create_context(const image_t* tex);
void					renderer_release(gfx_context_t* ctx);
void					renderer_begin(gfx_context_t* ctx, int width, int height);
void					renderer_quad(gfx_context_t* ctx, vec2_t sv, vec2_t st, vec2_t ev, vec2_t et, color4_t col);
void					renderer_end(gfx_context_t* ctx);

#endif // BOXWORLD_H
