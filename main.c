#define GLFW_INCLUDE_NONE 1
#include <GLFW/glfw3.h>

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
#include "emuGLES2/emuGLES2.h"

#include "boxworld.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

static char				bworld_error_string[MAX_ERROR_LENGTH]	= {0};
static BOXWORLD_ERROR	bworld_error							= NO_ERROR;

void*
boxworld_error(BOXWORLD_ERROR err, const char* string) {
	size_t sl		= strlen(string);

	if( sl >= MAX_ERROR_LENGTH ) {
		exit(1);
	}

	bworld_error	= err;
	memcpy(bworld_error_string, string, sl);

	return NULL;
}

BOXWORLD_ERROR		boxworld_error_number()	{ return bworld_error; }
extern const char*	boxworld_error_string()	{ return bworld_error_string; }

static void
error_callback(int error, const char* description) {
	fputs(description, stderr);
}

static void
key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}

int
main(void) {
	GLFWwindow*		window	= NULL;
	image_t*		tex	= NULL;
	font_t*			fnt	= NULL;
	gfx_context_t*	ctx	= NULL;
	static uint32	chars[128 - 32];
	uint32			i;

	glfwSetErrorCallback(error_callback);

	if (!glfwInit())
		exit(EXIT_FAILURE);

	window = glfwCreateWindow(640, 480, "Simple example", NULL, NULL);
	if (!window) {
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, key_callback);

	tex	= image_load_png("boxworld.png");
	if( !tex ) {
		fprintf(stderr, "unable to read texture:\n%s", boxworld_error_string());
		exit(EXIT_FAILURE);
	}


	for( i = 32; i < 128; ++i ) {
		chars[i - 32]	= i;
	}

	fnt	= font_bake("DroidSans.ttf", 16, true, true, true, 128 - 32, chars);

	ctx	= renderer_create_context(fnt->atlas->baked_image);

	while (!glfwWindowShouldClose(window)) {
		int width, height;
		int	c;
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);
		glClearColor(0.5f, 0.5f, 0.5f, 0.5f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		renderer_begin(ctx, width, height);
		renderer_quad(ctx,
					  vec2(0.0f, 0.0f), vec2(0.0f, 0.0f),
					  vec2(fnt->atlas->baked_image->width, fnt->atlas->baked_image->height), vec2(1.0f, 1.0f),
					  color4(1.0f, 1.0f, 1.0f, 1.0f));


		const uint8* str = (const uint8*)"Hello World!\nThis is a test";
		font_render_utf8(ctx, fnt, vec2(0.0f, 384.0f), (uint32)strlen((const char*)str), str, color4(1.0f, 1.0f, 1.0f, 1.0f));

		renderer_end(ctx);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	renderer_release(ctx);
	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}
