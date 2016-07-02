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
#include <assert.h>

#include "c99-3d-math/3dmath.h"
#include "emuGLES2/emuGLES2.h"

#define ARRAY_TYPE(name, type)	\
			typedef struct { \
				size_t	count; \
				size_t  max; \
				type*	array; \
			} name ## _t; \
			static INLINE name ## _t		name ## _new()				{ name ## _t v; v.count = 0; v.max = 0; v.array = NULL; return v; } \
			static INLINE void		name ## _release(name ## _t* v)		{ v->count = 0; v->max = 0; free(v->array); v->array = NULL; } \
			static INLINE type		name ## _get(name ## _t* v, size_t idx)	{ return v->array[idx]; } \
			static INLINE void		name ## _set(name ## _t* v, size_t idx, type t)	{ v->array[idx] = t; } \
			static INLINE type		name ## _pop(name ## _t* v)		{ type t = v->array[--(v->count)]; return t; } \
			static INLINE void		name ## _push(name ## _t* v, type e)	{ if( v->count == v->max ) { \
														   v->max = v->max ? (v->max) << 1 : 2; \
														   v->array = (type*)realloc(v->array, sizeof(type) * v->max);	\
													  } \
													  v->array[(v->count)++] = e; \
													} \
			static INLINE name ## _t		name ## _copy(name ## _t* orig)		{ name ## _t dest; \
													  dest.count = orig->count; \
													  dest.max = orig->max; \
													  dest.array = (type*)malloc(sizeof(type) * orig->max); \
													  memcpy(dest.array, orig->array, sizeof(type) * orig->max); \
													  return dest; \
													} \
			static INLINE void		name ## _resize(name ## _t* v, size_t s){ v->max = s; v->count = MIN(v->count, s); v->array = (type*)realloc(v->array, sizeof(type) * s); }

typedef enum {
	NO_ERROR		= 0,
	FILE_NOT_FOUND,
	INVALID_FORMAT,
	NOT_ENOUGH_MEMORY,
	UNSUPPORTED,
	LOAD_FAILED,

	MAX_ERROR_LENGTH	= 2048
} BOXWORLD_ERROR;

extern void*			boxworld_error(BOXWORLD_ERROR err, const char* string);
extern BOXWORLD_ERROR	boxworld_error_number();
extern const char*		boxworld_error_string();

/*
 * image.c
 */

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

image_t*				image_allocate(uint32 width, uint32 height, PIXEL_FORMAT fmt);
void					image_release(image_t* img);
image_t*				image_load_png(const char* path);

/* TODO: these are slow to use for iteration, best case would be more granular function table */
color4b_t				image_get_pixelb(const image_t* img, uint32 x, uint32 y);
color4_t				image_get_pixelf(const image_t* img, uint32 x, uint32 y);

void					image_set_pixelb(image_t* img, uint32 x, uint32 y, color4b_t rgba);
void					image_set_pixelf(image_t* img, uint32 x, uint32 y, color4_t rgba);

typedef struct {
	image_t*		baked_image;
	uint32			image_count;
	rect_t*			coordinates;
} atlas_t;

atlas_t*				image_atlas_make(uint32 image_count, const image_t **images);
void					image_atlas_release(atlas_t* atlas);

/*
 * render.c
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

/*
 * utf8.c
 */
typedef enum {
	UTF8_ACCEPT	= 0,
	UTF8_REJECT = 12
} UTF8_STATE;

UTF8_STATE				utf8_decode(uint32 *state, uint32* codep, uint32 byte);

/*
 * font.c
 */
typedef struct {
	uint32			code_point;
	rect_t			tcoords;
	vec2_t			start;
	float			advance;
} char_info_t;

typedef struct {
	uint32			size;
	uint32			char_count;
	char_info_t*	chars;	/* chars are sorted by code point */
	atlas_t*		atlas;
} font_t;

font_t*					font_bake(const char* filename, uint32 size, bool use_hint, bool force_autohinter, bool anti_alias, uint32 cp_count, uint32* cps);
void					font_release(font_t* fnt);
uint32					font_find_codepoint_index(const font_t* fnt, uint32 cp);
vec2_t					font_render_char(gfx_context_t* ctx, const font_t* fnt, vec2_t pos, uint32 cp, color4_t col);
vec2_t					font_render_string(gfx_context_t* ctx, const font_t *fnt, vec2_t pos, uint32 str_len, const uint32 *cps, color4_t col);
vec2_t					font_render_utf8(gfx_context_t* ctx, const font_t* fnt, vec2_t pos, uint32 str_len, const uint8* cps, color4_t col);

/*
 * level.c
 */
typedef enum {
	ACT_NONE,
	ACT_PLAYER,
	ACT_BOX
} ACTOR;

typedef enum {
	BG_EMPTY,
	BG_WALL,
	BG_GROUND,
	BG_PLACE,
} BACKGROUND;

typedef struct {
	ACTOR		actor;
	BACKGROUND	bg;
} cell_t;

typedef struct {
	uint32		width;
	uint32		height;
	cell_t*		cells;
} level_t;

typedef struct {
	uint32		count;
	level_t*	levels;
} level_collection_t;

typedef enum {
	KEY_UP,
	KEY_RIGHT,
	KEY_DOWN,
	KEY_LEFT,
	KEY_UNDO,
	KEY_REDO,
	KEY_EXIT,
} KEY;

ARRAY_TYPE(key_array, KEY)

typedef struct {
	uint32		player_pos;
	uint32		current_level;
	level_t		current;
	key_array_t	keys;
} game_state_t;

void					game_next_state(game_state_t* state, KEY key);


#endif // BOXWORLD_H
