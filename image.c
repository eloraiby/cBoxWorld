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
image_allocate(uint32 width, uint32 height, PIXEL_FORMAT fmt) {
	uint32		ps	= 0;
	image_t*	ret	= NULL;

	switch(fmt) {
	case PF_A8		: ps = 1; break;
	case PF_R8G8B8	: ps = 3; break;
	case PF_R8G8B8A8: ps = 4; break;
	default:
		fprintf(stderr, "ERROR: image_allocate: unsupported input format 0x%X\n", fmt);
		return NULL;
	}

	ret	= (image_t*)malloc(sizeof(image_t));
	assert( NULL != ret );

	ret->width	= width;
	ret->height	= height;
	ret->format	= fmt;
	ret->pixels	= malloc(width * height * ps);
	assert( NULL != ret->pixels );

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
		fprintf(stderr, "ERROR: load_png: %s not found\n", path);
		return NULL;
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

		fprintf(stderr, "ERROR: load_png: %s: invalid PNG format\n", path);
	}

	/* Allocate/initialize the memory
	* for image information. REQUIRED. */
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		fclose(fp);
		png_destroy_read_struct(&png_ptr, NULL, NULL);

		fprintf(stderr, "ERROR: load_png: %s: not enough memory or format not supported\n", path);
		return NULL;
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
		fprintf(stderr, "ERROR: load_png: inconsistant file %s\n", path);
		return NULL;
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
		memcpy(&(buff[width_size * r]), row_pointers[r], width_size);
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
find_best_size(uint32 img_count, const image_t** imgs) {
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

		stbrp_init_target(&ctx, (sint32)width, (sint32)width, nodes, (sint32)width * 2);
		stbrp_pack_rects(&ctx, rects, (sint32)img_count);

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

color4b_t
image_get_pixelb(const image_t* img, uint32 x, uint32 y) {
	uint32		offset	= 0;
	uint8*		pixels	= (uint8*)img->pixels;
	switch(img->format) {
	case PF_A8:
		offset	= img->width * y + x;
		return color4b(0xFF, 0xFF, 0xFF, pixels[offset]);
	case PF_R8G8B8:
		offset	= (img->width * y + x) * 3;
		return color4b(pixels[offset + 0], pixels[offset + 1], pixels[offset + 2], 0xFF);
	case PF_R8G8B8A8:
		offset	= (img->width * y + x) * 4;
		return color4b(pixels[offset + 0], pixels[offset + 1], pixels[offset + 2], pixels[offset + 3]);
	}
}

color4_t
image_get_pixelf(const image_t* img, uint32 x, uint32 y) {
	uint32		offset	= 0;
	uint8*		pixels	= (uint8*)img->pixels;
	float		r, g, b, a;

	switch(img->format) {
	case PF_A8:
		offset	= img->width * y + x;
		a		= pixels[offset];
		return color4(1.0f, 1.0f, 1.0f, a / 255.0f);
	case PF_R8G8B8:
		offset	= (img->width * y + x) * 3;
		r		= pixels[offset + 0];
		g		= pixels[offset + 1];
		b		= pixels[offset + 2];
		return color4(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
	case PF_R8G8B8A8:
		offset	= (img->width * y + x) * 4;
		r		= pixels[offset + 0];
		g		= pixels[offset + 1];
		b		= pixels[offset + 2];
		a		= pixels[offset + 3];
		return color4(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
	}
}

void
image_set_pixelb(image_t* img, uint32 x, uint32 y, color4b_t rgba) {
	uint32		offset	= 0;
	uint8*		pixels	= (uint8*)img->pixels;
	switch(img->format) {
	case PF_A8:
		offset	= img->width * y + x;
		pixels[offset]	= rgba.a;
		break;
	case PF_R8G8B8:
		offset	= (img->width * y + x) * 3;
		pixels[offset + 0]	= rgba.r;
		pixels[offset + 1]	= rgba.g;
		pixels[offset + 2]	= rgba.b;
		break;
	case PF_R8G8B8A8:
		offset	= (img->width * y + x) * 4;
		pixels[offset + 0]	= rgba.r;
		pixels[offset + 1]	= rgba.g;
		pixels[offset + 2]	= rgba.b;
		pixels[offset + 3]	= rgba.a;
		break;
	}
}

void
image_set_pixelf(image_t* img, uint32 x, uint32 y, color4_t rgbaf) {
	uint32		offset	= 0;
	uint8*		pixels	= (uint8*)img->pixels;
	color4b_t	rgba	= color4b((uint8)(rgbaf.r * 255.0f), (uint8)(rgbaf.g * 255.0f), (uint8)(rgbaf.b * 255.0f), (uint8)(rgbaf.a * 255.0f));
	switch(img->format) {
	case PF_A8:
		offset	= img->width * y + x;
		pixels[offset]	= rgba.a;
		break;
	case PF_R8G8B8:
		offset	= (img->width * y + x) * 3;
		pixels[offset + 0]	= rgba.r;
		pixels[offset + 1]	= rgba.g;
		pixels[offset + 2]	= rgba.b;
		break;
	case PF_R8G8B8A8:
		offset	= (img->width * y + x) * 4;
		pixels[offset + 0]	= rgba.r;
		pixels[offset + 1]	= rgba.g;
		pixels[offset + 2]	= rgba.b;
		pixels[offset + 3]	= rgba.a;
		break;
	}
}

atlas_t*
image_atlas_make(uint32 image_count, const image_t** images) {
	stbrp_rect*	rects	= NULL;
	stbrp_node*	nodes	= NULL;
	stbrp_context	ctx;
	uint32		r;
	uint32		best_size;
	image_t*	tex		= NULL;
	atlas_t*	atlas	= NULL;
	rect_t*		drects	= NULL;

	/* try to find the best texture size */
	best_size	= find_best_size(image_count, images);

	/* create the texture and fill in the pixels */
	tex	= image_allocate(best_size, best_size, PF_R8G8B8A8);
	assert( NULL != tex );

	/* image to rect */
	rects	= (stbrp_rect*)malloc(sizeof(stbrp_rect) * image_count);
	assert( NULL != rects );

	for( r = 0; r < image_count; ++r ) {
		rects[r]	= image_to_rect(r, images[r]);
	}

	/* nodes */
	nodes	= (stbrp_node*)malloc(sizeof(stbrp_node) * best_size * 2);
	assert( NULL != nodes );

	memset(nodes, 0, sizeof(stbrp_node) * best_size * 2);

	/* desitnation rectangles */
	drects	= (rect_t*)malloc(sizeof(rect_t) * image_count);
	assert( NULL != drects );

	memset(drects, 0, sizeof(rect_t) * image_count);

	/* pack */
	stbrp_init_target(&ctx, (int)best_size, (int)best_size, nodes, (int)best_size * 2);
	stbrp_pack_rects(&ctx, rects, (int)image_count);

	free(nodes);

	/* copy the rectangle */
	for( r = 0; r < image_count; ++r ) {
		uint32	y;
		uint32	h	= images[r]->height;
		uint32	w	= images[r]->width;

		assert( rects[r].was_packed );

		drects[r].x	= rects[r].x;
		drects[r].y	= rects[r].y;
		drects[r].width	= rects[r].w;
		drects[r].height= rects[r].h;

		for( y = 0; y < h ; ++y ) {
			uint32	x;
			for( x = 0; x < w; ++x ) {
				color4b_t	src	= image_get_pixelb(images[r], x, y);
				image_set_pixelb(tex, rects[r].x + x, rects[r].y + y, src);
			}
		}
	}

	/* release resources */
	free(rects);

	/* final result */
	atlas	= (atlas_t*)malloc(sizeof(atlas_t));
	assert( NULL != atlas );

	atlas->baked_image	= tex;
	atlas->coordinates	= drects;
	atlas->image_count	= image_count;
	return atlas;
}

void
image_atlas_release(atlas_t* atlas) {
	image_release(atlas->baked_image);
	free(atlas->coordinates);
	free(atlas);
}
