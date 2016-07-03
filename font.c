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

#include "boxworld.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

/* intermediate char info */
typedef struct {
	uint32		code_point;	/* UTF code point */
	ivec2_t		box_min;	/* char bounding box min position */
	ivec2_t		box_max;	/* char bounding box max position */
	float		advance;	/* number of pixels to add horizontally after this character */
	image_t*	img;
} glyph_info_t;

/* compiled font result */
typedef struct {
	uint32			char_count;
	glyph_info_t*	chars;
	vec2_t			max_char_size;
} font_result_t;

static int
unpack_bits(uint8 byte, int max_w, uint32* offset, image_t* dest) {
	int	w	= 0;
	int		i;
	for( i = 7; i >= 0 && w < max_w; i--, w++ ) {
		if( byte & 0x80 ) {
			((uint8*)dest->pixels)[*offset]	= 0xFF;
		} else {
			((uint8*)dest->pixels)[*offset]	= 0x00;
		}
		byte <<= 1;
		++(*offset);
	}
	return w;
}

static font_result_t*
font_result_bake(FT_Library lib, const char* path, uint32 size, bool use_hint, bool force_autohinter, bool anti_alias, uint32 char_count, const uint32* chars) {
	FT_Error		error;
	FT_Face			face;
	FT_GlyphSlot	slot;
	FT_Int32		load_flags;
	FT_Render_Mode	render_flags;

	vec2_t			max_char_size	= vec2(-FLT_MAX, -FLT_MAX);
	uint32			c;
	font_result_t*	result	= NULL;

	char		error_buff[MAX_ERROR_LENGTH]	= {0};

	error	= FT_New_Face(lib, path, 0, &face);
	if( error ) {
		sprintf(error_buff, "font_bake: failed to load font %s", path);
		return (font_result_t*)boxworld_error(LOAD_FAILED, error_buff);
	}

	error	= FT_Set_Pixel_Sizes(face, 0, size);
	if( error ) {
		/* release resources */
		FT_Done_Face(face);

		sprintf(error_buff, "font_bake: failed to set font size to %d pixels", size);
		return (font_result_t*)boxworld_error(LOAD_FAILED, error_buff);
	}

	slot			= face->glyph;
	load_flags		= FT_LOAD_DEFAULT;
	render_flags	= FT_RENDER_MODE_NORMAL;

	if( !use_hint ) {
		load_flags	|= FT_LOAD_NO_HINTING;
	}

	if( force_autohinter ) {
		load_flags	|= FT_LOAD_FORCE_AUTOHINT;
	} else {
		load_flags	|= FT_LOAD_NO_AUTOHINT;
	}

	if( !anti_alias ) {
		render_flags	= FT_RENDER_MODE_MONO;
	}

	glyph_info_t*	cis	= (glyph_info_t*)malloc(char_count * sizeof(glyph_info_t));
	memset(cis, 0, sizeof(glyph_info_t) * char_count);

	for( c = 0; c < char_count; ++c ) {
		FT_Glyph	glyph;
		FT_BBox		box;
		uint32		width, height;
		uint32		glyph_index	= FT_Get_Char_Index(face, (uint32)(chars[c]));

		cis[c].code_point	= (uint32)chars[c];

		error	= FT_Load_Glyph(face, glyph_index, load_flags);
		if( error ) {
			printf("WARNING: font_bake: couldn't load char 0x%X\n", chars[c]);
			continue;
		}

		error	= FT_Render_Glyph(slot, render_flags);
		if( error ) {
			printf("WARNING: font_bake: couldn't render char 0x%X\n", chars[c]);
			continue;
		}

		FT_Get_Glyph(slot, &glyph);
		FT_Glyph_Get_CBox(glyph, FT_GLYPH_BBOX_TRUNCATE, &box);

		width	= slot->bitmap.width;
		height	= slot->bitmap.rows;
		cis[c].advance	= slot->advance.x >> 6;
		cis[c].box_min	= ivec2((int)box.xMin, (int)box.yMin);
		cis[c].box_max	= ivec2((int)box.xMax, (int)box.yMax);
		cis[c].img	= image_allocate(width, height, PF_A8);

		assert( cis[c].img );

		max_char_size.x	= MAX(max_char_size.x, box.xMax - box.xMin);
		max_char_size.y	= MAX(max_char_size.y, box.yMax - box.yMin);

		if( slot->bitmap.pixel_mode == FT_PIXEL_MODE_MONO ) {
			uint32	offset	= 0;
			for( uint32 y = 0; y < slot->bitmap.rows; ++y ) {
				int	w	= (int)slot->bitmap.width;
				for( uint32 x = 0; x < (uint32)slot->bitmap.pitch; ++x ) {
					unpack_bits(slot->bitmap.buffer[x + y * (uint32)slot->bitmap.pitch], w, &offset, cis[c].img);
					w	-= 8;
				}
			}
		} else if( slot->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY ) {
			for( uint32 y = 0; y < slot->bitmap.rows; ++y ) {
				for( uint32 x = 0; x < (uint32)slot->bitmap.width; ++x ) {
					((uint8*)cis[c].img->pixels)[x + y * width]	= slot->bitmap.buffer[x + y * (uint32)slot->bitmap.pitch];
				}
			}
		} else {
			fprintf(stderr, "font_result_bake: unhandled pixel mode!!!\n");
			exit(1);
		}

		FT_Done_Glyph(glyph);
	}

	/* create the final structure */
	result	= (font_result_t*)malloc(sizeof(font_result_t));
	memset(result, 0, sizeof(font_result_t));

	result->char_count	= char_count;
	result->chars		= cis;
	result->max_char_size	= max_char_size;

	FT_Done_Face(face);

	return result;
}

static void
font_result_release(font_result_t* res) {
	for( uint32 c = 0; c < res->char_count; ++c ) {
		image_release(res->chars[c].img);
	}

	free(res->chars);
	free(res);
}


int
char_font_compare(const void* a, const void* b) {
	const char_info_t*	ca	= (const char_info_t*)a;
	const char_info_t*	cb	= (const char_info_t*)b;
	return (int)(ca->code_point - cb->code_point);
}

font_t*
font_bake(const char* filename,
		  uint32 size,
		  bool use_hint,
		  bool force_autohinter,
		  bool anti_alias,
		  uint32 cp_count,
		  uint32* cps)
{
	font_t*			result	= NULL;
	font_result_t*	ires	= NULL;
	atlas_t*		atlas	= NULL;

	FT_Error		fterror;
	FT_Library		ftlib;

	fterror	= FT_Init_FreeType(&ftlib);
	if( fterror ) {
		return (font_t*)boxworld_error(LOAD_FAILED, "font_bake: unable to load freetype library, bailing...");
	}

	ires	= font_result_bake(ftlib, filename, size, use_hint, force_autohinter, anti_alias, cp_count, cps);
	if( !ires ) {
		return NULL;
	}

	/* build atlas */
	image_t**	imgs	= (image_t**)malloc(sizeof(image_t*) * cp_count);
	assert( imgs != NULL );
	memset(imgs, 0, sizeof(image_t*) * cp_count);

	for( uint32 c = 0; c < cp_count; ++c ) {
		imgs[c]	= ires->chars[c].img;
	}

	atlas	= image_atlas_make(cp_count, imgs);

	free(imgs);

	/* create font */
	result	= (font_t*)malloc(sizeof(font_t));
	assert( result != NULL );

	/* create the char info */
	result->atlas		= atlas;
	result->char_count	= ires->char_count;

	result->chars		= (char_info_t*)malloc(sizeof(char_info_t) * ires->char_count);
	assert( result->chars );

	/* set the char info */
	for( uint32 c = 0; c < ires->char_count; ++c ) {
		result->chars[c].advance	= ires->chars[c].advance;
		result->chars[c].start		= vec2(ires->chars[c].box_min.x,
										   ires->chars[c].box_min.y);
		result->chars[c].code_point	= cps[c];
		result->chars[c].tcoords	= rect(atlas->coordinates[c].x, atlas->coordinates[c].y,
										   atlas->coordinates[c].width - 1, atlas->coordinates[c].height - 1);
	}

	/* sort by code point by increasing order */
	qsort(result->chars, cp_count, sizeof(char_info_t), char_font_compare);

	result->size	= size;

	/* release resource */
	font_result_release(ires);
	FT_Done_FreeType(ftlib);
	return result;
}

void
font_release(font_t* fnt) {
	image_atlas_release(fnt->atlas);
	free(fnt->chars);
	free(fnt);
}

/* binary search for code point */
static uint32
rec_find_codepoint_index(const font_t* fnt, uint32 cp, uint32 left, uint32 right) {
	uint32	cl	= fnt->chars[left].code_point;
	if( cp == cl ) {
		return left;
	} else if( cp < cl ) {
		return (uint32)-1;
	} else {
		uint32	cr	= fnt->chars[right].code_point;
		if( cp == cr ) {
			return right;
		} else if( cp > cr ) {
			return (uint32)-1;
		} else {
			uint32	middle	= (left + right) >> 1;
			if( middle != left ) {
				uint32	cm	= fnt->chars[middle].code_point;
				if( cp > cm ) {
					return rec_find_codepoint_index(fnt, cp, middle, right);
				} else {
					return rec_find_codepoint_index(fnt, cp, left, middle);
				}
			} else {
				return (uint32)-1;
			}
		}
	}
}

uint32
find_codepoint_index(const font_t* fnt, uint32 cp) {
	return rec_find_codepoint_index(fnt, cp, 0, fnt->char_count - 1);
}

vec2_t
font_render_char(gfx_context_t* ctx, const font_t* fnt, vec2_t pos, uint32 cp, color4_t col) {
	uint32	cp_index	= find_codepoint_index(fnt, cp);

	if( cp_index != (uint32)-1 ) {
		float	tw	= fnt->atlas->baked_image->width;
		float	th	= fnt->atlas->baked_image->width;

		float	tsx	= fnt->chars[cp_index].tcoords.x / tw;
		float	tsy	= fnt->chars[cp_index].tcoords.y / th;
		float	tex	= tsx + fnt->chars[cp_index].tcoords.width / tw;
		float	tey	= tsy + fnt->chars[cp_index].tcoords.height/ th;

		vec2_t	start	= vec2_add(fnt->chars[cp_index].start, pos);

		float	w	= fnt->chars[cp_index].tcoords.width;
		float	h	= fnt->chars[cp_index].tcoords.height;

		renderer_quad(ctx,
					  vec2_add(start, vec2(0, -h)), vec2(tsx, tsy),
					  vec2_add(start, vec2(w, 0)), vec2(tex, tey),
					  col);

		return vec2_add(pos, vec2(fnt->chars[cp_index].advance, 0.0f));
	} else {
		return pos;
	}
}

vec2_t
font_render_string(gfx_context_t* ctx, const font_t* fnt, vec2_t pos, uint32 str_len, const uint32* cps, color4_t col) {
	vec2_t	ret	= pos;

	for( uint32 c = 0; c < str_len; ++c ) {
		if( (uint32)'\n' == cps[c] ) {
			ret.x	= pos.x;
			ret.y	= pos.y + fnt->size;
		}
		ret	= font_render_char(ctx, fnt, ret, cps[c], col);
	}
	return ret;
}

vec2_t
font_render_utf8(gfx_context_t* ctx, const font_t* fnt, vec2_t pos, uint32 str_len, const uint8* cps, color4_t col) {
	vec2_t	ret	= pos;
	uint32	state	= 0;
	uint32	cp	= 0;

	for( uint32 c = 0; c < str_len; ++c ) {
		if( UTF8_ACCEPT == utf8_decode(&state, &cp, cps[c]) ) {
			if( (uint32)'\n' == cps[c] ) {
				ret.x	= pos.x;
				ret.y	= pos.y + fnt->size;
			}
			ret	= font_render_char(ctx, fnt, ret, cp, col);
		}
	}
	return ret;
}
