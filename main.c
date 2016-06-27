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
	gfx_context_t*	ctx	= NULL;

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


	ctx	= renderer_create_context(tex);

	while (!glfwWindowShouldClose(window))
	{
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);
		glClearColor(0.5f, 0.5f, 0.5f, 0.5f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		renderer_begin(ctx, width, height);
		renderer_quad(ctx,
					  vec2(0.0f, 0.0f), vec2(0.0f, 0.0f),
					  vec2(width * 0.5f, height * 0.5f), vec2(1.0f, 1.0f),
					  color4(1.0f, 1.0f, 1.0f, 1.0f));
		renderer_end(ctx);
//		glMatrixMode(GL_PROJECTION);
//		glLoadIdentity();
//		glOrtho(-ratio, ratio, -1.f, 1.f, 1.f, -1.f);
//		glMatrixMode(GL_MODELVIEW);
//		glLoadIdentity();
//		glRotatef((float) glfwGetTime() * 50.f, 0.f, 0.f, 1.f);
//		glBegin(GL_TRIANGLES);
//		glColor3f(1.f, 0.f, 0.f);
//		glVertex3f(-0.6f, -0.4f, 0.f);
//		glColor3f(0.f, 1.f, 0.f);
//		glVertex3f(0.6f, -0.4f, 0.f);
//		glColor3f(0.f, 0.f, 1.f);
//		glVertex3f(0.f, 0.6f, 0.f);
//		glEnd();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	renderer_release(ctx);
	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}
