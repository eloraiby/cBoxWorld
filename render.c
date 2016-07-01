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

gfx_context_t*
renderer_create_context(const image_t* tex) {
	GL_ENUM			pf;
	gfx_context_t*	ctx	= (gfx_context_t*)malloc(sizeof(gfx_context_t));
	char			infoLog[2048]	= {0};
	int				infoLen			= 0;

	if( NULL == ctx ) {
		return (gfx_context_t*)boxworld_error(NOT_ENOUGH_MEMORY, "renderer_create_context: not enough memory");
	}

	memset(ctx, 0, sizeof(gfx_context_t));

	glGenTextures(1, &(ctx->texture));
	glBindTexture(GL_TEXTURE_2D, ctx->texture);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	switch(tex->format) {
	case PF_A8		: pf	= GL_ALPHA; break;
	case PF_R8G8B8	: pf	= GL_RGB;	break;
	case PF_R8G8B8A8: pf	= GL_RGBA;	break;
	}

	glTexImage2D(GL_TEXTURE_2D, 0, pf, tex->width, tex->height, 0, pf, GL_UNSIGNED_BYTE, tex->pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	ctx->program = glCreateProgram();

	const char * vs =
			"#version 120\n"
			"uniform highp vec2 Viewport;\n"
			"attribute highp vec2 VertexPosition;\n"
			"attribute highp vec2 VertexTexCoord;\n"
			"attribute highp vec4 VertexColor;\n"
			"varying highp vec2 texCoord;\n"
			"varying highp vec4 vertexColor;\n"
			"void main()\n"
			"{\n"
			"    vertexColor = VertexColor;\n"
			"    texCoord = VertexTexCoord;\n"
			"    gl_Position = vec4(VertexPosition * 2.0 / Viewport - 1.0, 0.0, 1.0);\n"
			"}\n";
	GLuint vso = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vso, 1, (const char **)  &vs, NULL);
	glCompileShader(vso);
	glGetShaderInfoLog(vso, 2048, &infoLen, infoLog);
	printf("vs shader log: %s\n", infoLog);
	glAttachShader(ctx->program, vso);

	const char * fs =
			"#version 120\n"
			"varying highp vec2 texCoord;\n"
			"varying highp vec4 vertexColor;\n"
			"uniform sampler2D Texture;\n"
			"void main()\n"
			"{\n"
			"    gl_FragColor = texture2D(Texture, texCoord) * vertexColor;\n"
			"}\n";
	GLuint fso = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(fso, 1, (const char **) &fs, NULL);
	glCompileShader(fso);
	glGetShaderInfoLog(fso, 2048, &infoLen, infoLog);
	printf("fs shader log: %s\n", infoLog);
	glAttachShader(ctx->program, fso);

	glLinkProgram(ctx->program);
	glGetProgramInfoLog(ctx->program, 2048, &infoLen, infoLog);
	printf("programr log: %s\n", infoLog);
	glDeleteShader(vso);
	glDeleteShader(fso);

	glUseProgram(ctx->program);
	ctx->uniViewport	= glGetUniformLocation(ctx->program, "Viewport");
	ctx->uniTexture		= glGetUniformLocation(ctx->program, "Texture");

	ctx->attrPosition	= glGetAttribLocation(ctx->program, "VertexPosition");
	ctx->attrTexCoord	= glGetAttribLocation(ctx->program, "VertexTexCoord");
	ctx->attrColor		= glGetAttribLocation(ctx->program, "VertexColor");

	glUseProgram(0);

	glGenBuffers(1, &(ctx->vbo));

	glBindBuffer(GL_ARRAY_BUFFER, ctx->vbo);
	glBufferData(GL_ARRAY_BUFFER, MAX_QUADS * sizeof(render_quad_t), ctx->quads, GL_DYNAMIC_DRAW);

	return ctx;
}

void
renderer_release(gfx_context_t* ctx) {
	if( ctx->texture ) {
		glDeleteTextures(1, &(ctx->texture));
		ctx->texture = 0;
	}

	if( ctx->vbo ) {
		glDeleteBuffers(1, &(ctx->vbo));
		ctx->vbo	= 0;
	}

	if( ctx->program ) {
		glDeleteProgram(ctx->program);
		ctx->program = 0;
	}

}

void
renderer_begin(gfx_context_t* ctx, int width, int height) {
	glViewport(0, 0, width, height);
	glUseProgram(ctx->program);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, ctx->texture);
	glUniform2f(ctx->uniViewport, (float) width, (float) height);
	glUniform1i(ctx->uniTexture, 0);

	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glDisable(GL_CULL_FACE);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

static void
flush(gfx_context_t* ctx) {
	glEnableVertexAttribArray(ctx->attrPosition);
	glEnableVertexAttribArray(ctx->attrTexCoord);
	glEnableVertexAttribArray(ctx->attrColor);

	/* TODO: this is highly inefficient */
	glBindBuffer(GL_ARRAY_BUFFER, ctx->vbo);

	/* is this needed ? */
	glBufferSubData(GL_ARRAY_BUFFER, 0, ctx->numQuads * sizeof(render_quad_t), ctx->quads);

	glVertexAttribPointer(ctx->attrPosition, 2, GL_FLOAT, GL_FALSE, sizeof(render_vertex_t), (void*)0);
	glVertexAttribPointer(ctx->attrTexCoord, 2, GL_FLOAT, GL_FALSE, sizeof(render_vertex_t), (void*)sizeof(vec2_t));
	glVertexAttribPointer(ctx->attrColor,    4, GL_FLOAT, GL_FALSE, sizeof(render_vertex_t), (void*)(sizeof(vec2_t) + sizeof(vec2_t)));

	glDrawArrays(GL_TRIANGLES, 0, 6 * ctx->numQuads);

	glDisableVertexAttribArray(ctx->attrPosition);
	glDisableVertexAttribArray(ctx->attrTexCoord);
	glDisableVertexAttribArray(ctx->attrColor);

	ctx->numQuads	= 0;
}

void
renderer_quad(gfx_context_t* ctx, vec2_t sv, vec2_t st, vec2_t ev, vec2_t et, color4_t col) {
	vec2_t		v0, v1, v2, v3;
	vec2_t		t0, t1, t2, t3;

	float		x0 = sv.x, y0 = sv.y, x1 = ev.x, y1 = ev.y;
	float		tu0 = st.x, tv0 = st.y, tu1 = et.x, tv1 = et.y;

	if( ctx->numQuads + 1 > MAX_QUADS ) {
		/* flush */
		flush(ctx);
	}


	v0	= vec2(x0, y0);
	v1	= vec2(x1, y0);
	v2	= vec2(x1, y1);
	v3	= vec2(x0, y1);

	t0	= vec2(tu0, tv0);
	t1	= vec2(tu1, tv0);
	t2	= vec2(tu1, tv1);
	t3	= vec2(tu0, tv1);

	ctx->quads[ctx->numQuads][0].position	= v0;
	ctx->quads[ctx->numQuads][1].position	= v1;
	ctx->quads[ctx->numQuads][2].position	= v2;
	ctx->quads[ctx->numQuads][3].position	= v0;
	ctx->quads[ctx->numQuads][4].position	= v2;
	ctx->quads[ctx->numQuads][5].position	= v3;

	ctx->quads[ctx->numQuads][0].tex		= t0;
	ctx->quads[ctx->numQuads][1].tex		= t1;
	ctx->quads[ctx->numQuads][2].tex		= t2;
	ctx->quads[ctx->numQuads][3].tex		= t0;
	ctx->quads[ctx->numQuads][4].tex		= t2;
	ctx->quads[ctx->numQuads][5].tex		= t3;

	ctx->quads[ctx->numQuads][0].color		= col;
	ctx->quads[ctx->numQuads][1].color		= col;
	ctx->quads[ctx->numQuads][2].color		= col;
	ctx->quads[ctx->numQuads][3].color		= col;
	ctx->quads[ctx->numQuads][4].color		= col;
	ctx->quads[ctx->numQuads][5].color		= col;

	++(ctx->numQuads);
}

void
renderer_end(gfx_context_t* ctx) {
	flush(ctx);
	glUseProgram(0);
	glDepthMask(GL_TRUE);
}

