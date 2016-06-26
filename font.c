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

ARRAY_TYPE(pix_stream, uint8)

/* intermediate char info */
typedef struct {
	uint32		code_point;	/* UTF code point */
	ivec2_t		box_min;	/* char bounding box min position */
	ivec2_t		box_max;	/* char bounding box max position */
	uint32		width;		/* char width */
	uint32		height;		/* char height */
	float		advance;	/* number of pixels to add horizontally after this character */
	uint32		pixmap_offset;	/* pointer into pixmap */
} glyph_info_t;

/* compiled font result */
typedef struct {
	uint32			char_count;
	vec2_t			max_char_size;
	glyph_info_t*	chars;

	pix_stream_t	pixmap;	/* not really a pixmap, rather a stream of bytes where char_info points inside */

} font_result_t;

static uint32
unpack_bits(uint8 byte, uint32 max_w, pix_stream_t* dest) {
	uint32	w	= 0;
	for( int i = 7; i >= 0 && w < max_w; i--, w++ ) {
		if( byte & 0x80 ) { pix_stream_push(dest, 0xFF); }
		else { pix_stream_push(dest, 0x00); }
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
	pix_stream_t	pixmap;
	vec2_t			max_char_size	= vec2(-FLT_MAX, -FLT_MAX);
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

	pixmap	= pix_stream_new();

	for( uint32 c = 0; c < char_count; ++c ) {
		FT_Glyph	glyph;
		FT_BBox		box;
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

		cis[c].width	= slot->bitmap.width;
		cis[c].height	= slot->bitmap.rows;
		cis[c].advance	= slot->advance.x >> 6;
		cis[c].box_min	= ivec2((int)box.xMin, (int)box.yMin);
		cis[c].box_max	= ivec2((int)box.xMax, (int)box.yMax);
		cis[c].pixmap_offset	= (uint32)pixmap.count;

		max_char_size.x	= MAX(max_char_size.x, box.xMax - box.xMin);
		max_char_size.y	= MAX(max_char_size.y, box.yMax - box.yMin);

		if( !anti_alias ) {
			for( uint32 y = 0; y < slot->bitmap.rows; ++y ) {
				uint32	w	= slot->bitmap.width;
				for( uint32 x = 0; x < (uint32)slot->bitmap.pitch; ++x ) {
					unpack_bits(slot->bitmap.buffer[x + y * (uint32)slot->bitmap.pitch], w, &pixmap);
					w	-= 8;
				}
			}
		} else {
			for( uint32 y = 0; y < slot->bitmap.rows; ++y ) {
				for( uint32 x = 0; x < (uint32)slot->bitmap.width; ++x ) {
					pix_stream_push(&pixmap, slot->bitmap.buffer[x + y * (uint32)slot->bitmap.pitch]);
				}
			}
		}

		FT_Done_Glyph(glyph);
	}

	/* create the final structure */
	result	= (font_result_t*)malloc(sizeof(font_result_t));
	memset(result, 0, sizeof(font_result_t));

	result->pixmap		= pixmap;
	result->char_count	= char_count;
	result->chars		= cis;
	result->max_char_size	= max_char_size;

	FT_Done_Face(face);

	return result;
}

static void
font_result_release(font_result_t* res) {
	free(res->chars);
	pix_stream_release(&res->pixmap);
	free(res);
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

	FT_Error		fterror;
	FT_Library		ftlib;

	fterror	= FT_Init_FreeType(&ftlib);
	if( fterror ) {
		return (font_t*)boxworld_error(LOAD_FAILED, "font_bake: unable to load freetype library, bailing...");
	}

	ires	= font_result_bake(ftlib, filename, size, use_hint, force_autohinter, anti_alias, cp_count, cps);

	FT_Done_FreeType(ftlib);
	return result;
}

void
font_release(font_t* fnt) {

}

void
font_render_char(gfx_context_t* ctx, font_t* fnt, uint32 cp) {

}

void
font_render_string(gfx_context_t* ctx, font_t* fnt, uint32 strLen, uint32* cps) {

}
