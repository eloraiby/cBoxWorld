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
#include <png.h>
#include "stb/stb_rect_pack.h"

image_t*
image_allocate(int width, int height, PIXEL_FORMAT fmt) {
	int			ps	= 0;
	image_t*	ret	= NULL;

	switch(fmt) {
	case PF_A8		: ps = 1; break;
	case PF_R8G8B8	: ps = 3; break;
	case PF_R8G8B8A8: ps = 4; break;
	default:
		return boxworld_error(UNSUPPORTED, "image_allocate: unsupported input format");
	}

	ret	= (image_t*)malloc(sizeof(image_t));
	if( NULL == ret ) {
		free(ret);
		return boxworld_error(NOT_ENOUGH_MEMORY, "image_allocate: not enough memory");
	}

	ret->width	= width;
	ret->height	= height;
	ret->format	= fmt;
	ret->pixels	= malloc(width * height * ps);

	if( NULL == ret->pixels ) {
		free(ret);
		return boxworld_error(NOT_ENOUGH_MEMORY, "image_allocate: not enough memory");
	}

	memset(ret->pixels, 0, width * height * ps);
	return ret;
}

void
image_release(image_t* img) {
	free(img->pixels);
	free(img);
}


image_t*
image_load_png(const char* path) {
	png_structp		png_ptr;
	png_infop		info_ptr;
	unsigned int	sig_read	= 0;
	int				color_type, interlace_type;
	FILE*			fp;
	png_uint_32		width, height;
	int				bit_depth;
	unsigned int	row_bytes;
	PIXEL_FORMAT	sf			= PF_A8;
	uint32			pixel_size	= 0;
	uint32			width_size	= 0;
	image_t*		tex			= NULL;
	char*			buff		= NULL;
	png_bytepp		row_pointers;
	uint32			r;	/* row */

	if( (fp = fopen(path, "rb")) == NULL ) {
			return (image_t*)boxworld_error(FILE_NOT_FOUND, "load_png: file not found");
	}

	/* Create and initialize the png_struct
	* with the desired error handler
	* functions. If you want to use the
	* default stderr and longjump method,
	* you can supply NULL for the last
	* three parameters. We also supply the
	* the compiler header file version, so
	* that we know if the application
	* was compiled with a compatible version
	* of the library. REQUIRED
	*/
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if( png_ptr == NULL ) {
			fclose(fp);
			return (image_t*)boxworld_error(INVALID_FORMAT, "load_png: invalid PNG format");
	}

	/* Allocate/initialize the memory
	* for image information. REQUIRED. */
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
			fclose(fp);
			png_destroy_read_struct(&png_ptr, NULL, NULL);
			return (image_t*)boxworld_error(NOT_ENOUGH_MEMORY, "load_png: not enough memory or format supported");
	}

	/* Set error handling if you are
	* using the setjmp/longjmp method
	* (this is the normal method of
	* doing things with libpng).
	* REQUIRED unless you set up
	* your own error handlers in
	* the png_create_read_struct()
	* earlier.
	*/
	if (setjmp(png_jmpbuf(png_ptr))) {
			/* Free all of the memory associated
			* with the png_ptr and info_ptr */
			png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
			fclose(fp);
			/* If we get here, we had a
			* problem reading the file */
			return (image_t*)boxworld_error(INVALID_FORMAT, "load_png: inconsistant file");
	}
	/* Set up the output control if
	* you are using standard C streams */
	png_init_io(png_ptr, fp);
	/* If we have already
	* read some of the signature */
	png_set_sig_bytes(png_ptr, sig_read);
	/*
	* If you have enough memory to read
	* in the entire image at once, and
	* you need to specify only
	* transforms that can be controlled
	* with one of the PNG_TRANSFORM_*
	* bits (this presently excludes
	* dithering, filling, setting
	* background, and doing gamma
	* adjustment), then you can read the
	* entire image (including pixels)
	* into the info structure with this
	* call
	*
	* PNG_TRANSFORM_STRIP_16 |
	* PNG_TRANSFORM_PACKING forces 8 bit
	* PNG_TRANSFORM_EXPAND forces to
	* expand a palette into RGB
	*/
	png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_EXPAND, NULL);

	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, NULL, NULL);

	row_bytes = png_get_rowbytes(png_ptr, info_ptr);

	switch( color_type ) {
	case PNG_COLOR_TYPE_GRAY:
	case PNG_COLOR_TYPE_GRAY_ALPHA:
		sf	= PF_A8;
		pixel_size	= 1;
		break;
	case PNG_COLOR_TYPE_RGB:
		sf	= PF_R8G8B8;
		pixel_size	= 3;
		break;
	case PNG_COLOR_TYPE_RGB_ALPHA:
		sf	= PF_R8G8B8A8;
		pixel_size	= 4;
		break;
	}

	// align the opengl texture to 4 byte width
	width_size	= width * pixel_size;
	tex			= image_allocate(width, height, sf);

	if( !tex ) {
		/* Clean up after the read,
		* and free any memory allocated */
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		/* Close the file */
		fclose(fp);

		return tex;
	}

	buff			= (char*)tex->pixels;
	row_pointers	= png_get_rows(png_ptr, info_ptr);

	for( r = 0; r < height; r++ ) {
		// note that png is ordered top to
		// bottom, but OpenGL expect it bottom to top
		// so the order or swapped
		memcpy(&(buff[width_size * (height - 1 - r)]), row_pointers[r], width_size);
	}

	/* Clean up after the read,
	* and free any memory allocated */
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	/* Close the file */
	fclose(fp);

	return tex;
}

static stbrp_rect
image_to_rect(uint32 id, const image_t* img) {
	stbrp_rect	rect;
	rect.w	= (uint16) (img->width  + 1);
	rect.h	= (uint16) (img->height + 1);
	rect.id	= (uint16)id;
	rect.was_packed	= 0;
	rect.x	= 0;
	rect.y	= 0;
	return rect;
}

static uint32
find_best_size(uint32 img_count, const image_t* const* imgs) {
	static uint32	texture_size[] = { 128,	256, 512, 1024, 2048 };
	bool			success	= true;
	uint32			size	= 0;
	uint32			r;
	uint32			s;

	stbrp_rect*		rects	= (stbrp_rect*)malloc(sizeof(stbrp_rect) * img_count);
	for( r = 0; r < img_count; ++r ) {
		rects[r]	= image_to_rect(r, imgs[r]);
	}

	for( s = 0; s < sizeof(texture_size) / sizeof(uint32); ++s ) {
		stbrp_context	ctx;
		uint32			r;
		uint32			width	= texture_size[s];
		stbrp_node*		nodes	= (stbrp_node*)malloc(sizeof(stbrp_node) * width * 2);
		memset(nodes, 0, sizeof(stbrp_node) * width * 2);

		stbrp_init_target(&ctx, width, width, nodes, width * 2);
		stbrp_pack_rects(&ctx, rects, img_count);

		free(nodes);

		/* check if all rectangles were packed */
		success	= true;
		for( r = 0; r < img_count; ++r ) {
			if( !rects[r].was_packed ) {
				success	= false;
				size	= 0;
				break;
			}
		}

		if( success ) {
			size	= width;
			break;
		}
	}

	free(rects);

	if( !success ) return 0;
	else return size;
}
