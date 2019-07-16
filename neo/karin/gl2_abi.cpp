#include "gl2_abi.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "vkb.h"

#define VSHADER_FILE _HARMATTAN_RESC"glsl/qd3.vert"
#define FSHADER_FILE _HARMATTAN_RESC"glsl/qd3.frag"

enum {
	ATTR_VERTEX = 0,
	ATTR_TEXCOORD,
	U_MODELVIEWPROJECTIONMATRIX,
	U_FRAGMENTMAP0,
	U_ALPHA,
	TOTAL_LOCATION
};

GLfloat projection_matrix[16];
GLfloat modelview_matrix[16];
GLfloat mvp_matrix[16];
static GLuint locations[TOTAL_LOCATION];
static GLuint program;
static unsigned prog_inited;
static GLuint vshader;
static GLuint fshader;
static const char *Location_Names[] = {
	"attr_Vertex",
	"attr_TexCoord",
	"u_modelViewProjectionMatrix",
	"u_fragmentMap0",
	"u_alpha",
};

// OpenGL ES has not glPushAttrib/glPopAttrib
// Cull Face
static struct OpenGL_CullFace_Attrib
{
	GLboolean cull_face;
	GLint front_face;
	GLint cull_face_mode;
} cull_face_attrib;
// Texture 2D
static struct OpenGL_Texture2D_Attrib
{
	GLint active_texture;
	GLboolean texture2d;
	GLint bind_texture2d;
} texture2d_attrib;
// Blend
static struct OpenGL_Blend_Attrib
{
	GLint blend_src;
	GLint blend_dst;
	GLint blend_asrc;
	GLint blend_adst;
	GLboolean blend;
} blend_attrib;
// Depth Test
static struct OpenGL_DepthTest_Attrib
{
	GLboolean depth_test;
} depth_test_attrib;
// Transform(Matrix and Viewport)
static struct OpenGL_Transform_Attrib
{
	GLint viewport[4];
} transform_attrib;
// Current
static struct OpenGL_Current_Attrib
{
	GLuint program;
} current_attrib;
// Stencil Test
static struct OpenGL_StencilTest_Attrib
{
	GLboolean stencil_test;
} stencil_test_attrib;
/*
// Scissor
static struct OpenGL_Scissor_Attrib
{
	GLint scissor_box[4];
	GLboolean scissor_test;
} scissor_attrib;
*/

static void karinPrintMatrix(const float m[16])
{
	int i;
	int j;

	if(!m)
		return;

	for(i = 0; i < 4; i++)
	{
		for(j = 0; j < 4; j++)
		{
			printf("%f  ", m[i * 4 + j]);
		}
		printf("\n");
	}
}

static void karinSetupState(void)
{
	if(render_lock)
		return;
	// Get
	{
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture2d_attrib.bind_texture2d);
		texture2d_attrib.texture2d = glIsEnabled(GL_TEXTURE_2D);
		glGetIntegerv(GL_ACTIVE_TEXTURE, &texture2d_attrib.active_texture);

		blend_attrib.blend = glIsEnabled(GL_BLEND);
		glGetIntegerv(GL_BLEND_SRC_RGB, &blend_attrib.blend_src);
		glGetIntegerv(GL_BLEND_DST_RGB, &blend_attrib.blend_dst);
		glGetIntegerv(GL_BLEND_SRC_ALPHA, &blend_attrib.blend_asrc);
		glGetIntegerv(GL_BLEND_DST_ALPHA, &blend_attrib.blend_adst);

		depth_test_attrib.depth_test = glIsEnabled(GL_DEPTH_TEST);

		glGetIntegerv(GL_VIEWPORT, transform_attrib.viewport);

		stencil_test_attrib.stencil_test = glIsEnabled(GL_STENCIL_TEST);

		cull_face_attrib.cull_face = glIsEnabled(GL_CULL_FACE);
		glGetIntegerv(GL_FRONT_FACE, &cull_face_attrib.front_face);
		glGetIntegerv(GL_CULL_FACE_MODE, &cull_face_attrib.cull_face_mode);


		//glGetIntegerv(GL_SCISSOR_BOX, scissor_attrib.scissor_box);
		//scissor_attrib.scissor_test = glIsEnabled(GL_SCISSOR_TEST);
	}
	// Set
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glEnable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0);

		glDisable(GL_DEPTH_TEST);

		glDisable(GL_STENCIL_TEST);

		glEnable(GL_CULL_FACE);
		glFrontFace(GL_CCW);
		glCullFace(GL_BACK);

		//glDisable(GL_SCISSOR_TEST);
	}
	render_lock = btrue;
}

static void karinRestoreState(void)
{
	if(!render_lock)
		return;
	{
		glBindTexture(GL_TEXTURE_2D, texture2d_attrib.bind_texture2d);
		if(!texture2d_attrib.texture2d)
			glDisable(GL_TEXTURE_2D);
		glActiveTexture(texture2d_attrib.active_texture);


		if(!blend_attrib.blend)
			glDisable(GL_BLEND);
		glBlendFuncSeparate(blend_attrib.blend_src, blend_attrib.blend_dst, blend_attrib.blend_asrc, blend_attrib.blend_adst);

		if(depth_test_attrib.depth_test)
			glEnable(GL_DEPTH_TEST);

		glViewport(transform_attrib.viewport[0], transform_attrib.viewport[1], transform_attrib.viewport[2], transform_attrib.viewport[3]);

		if(stencil_test_attrib.stencil_test)
			glEnable(GL_STENCIL_TEST);

		if(!cull_face_attrib.cull_face)
			glDisable(GL_CULL_FACE);
		glFrontFace(cull_face_attrib.front_face);
		glCullFace(cull_face_attrib.cull_face_mode);

		//if(scissor_attrib.scissor_test)
			//glEnable(GL_SCISSOR_TEST);
	}
	render_lock = bfalse;
}

static char * karinLoadSource(const char *file)
{
	if(!file)
		return NULL;
	FILE *in = fopen(file, "r");
	if(!in)
		return NULL;
	fseek(in, 0, SEEK_END);
	long len = ftell(in);
	fseek(in, 0, SEEK_SET);
	char *src = calloc(len + sizeof(char), sizeof(char));
	fread(src, sizeof(char), len, in);
	fclose(in);
	//printf("%s\n", src);
	return src;
}

static GLboolean karinShaderIsCompiled(GLuint shader)
{
	GLint compiled;
	GLint len;
	char *log;

	if(!glIsShader(shader))
		return GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if(!compiled)
	{
		len = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
		if(len > 1)
		{
			log = calloc(len, sizeof(char));
			glGetShaderInfoLog(shader, len, NULL, log);
			fprintf(stderr, "%d - %s\n", shader, log);
			free(log);
		}
		return GL_FALSE;
	}
	return GL_TRUE;
}

static GLuint karinLoadShader(GLenum type, const char *src)
{
	GLuint shader;

	if(!src)
		return 0;
	shader = glCreateShader(type);
	if(shader == 0)
		return 0;
	glShaderSource(shader, 1, &src, NULL);
	glCompileShader(shader);
	if(karinShaderIsCompiled(shader))
		return shader;
	else
	{
		glDeleteShader(shader);
		return 0;
	}
}

static GLboolean karinProgramIsLinked(GLuint program)
{
	GLint linked;
	GLint len;
	char *log;

	if(!glIsProgram(program))
		return GL_FALSE;
	linked = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if(!linked)
	{
		len = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
		if(len > 1)
		{
			log = calloc(len, sizeof(char));
			glGetProgramInfoLog(program, len, NULL, log);
			fprintf(stderr, "%d - %s\n", program, log);
			free(log);
		}
		return GL_FALSE;
	}
	return GL_TRUE;
}

static GLboolean karinLinkShaderToProgram(GLuint program, GLuint vshader, GLuint fshader)
{
	GLboolean vis;
	GLboolean fis;

	if(!glIsProgram(program))
		return GL_FALSE;
	vis = glIsShader(vshader);
	fis = glIsShader(fshader);
	if(!vis && !fis)
		return GL_FALSE;
	if(vis)
		glAttachShader(program, vshader);
	if(fis)
		glAttachShader(program, fshader);
	glLinkProgram(program);
	return karinProgramIsLinked(program);
}

static GLuint karinLoadProgram(const char *vfile, const char *ffile, GLuint *rvs, GLuint *rfs)
{
	char *vsrc;
	char *fsrc;
	GLuint vshader;
	GLuint fshader;
	GLuint prog;

	if(!vfile || !ffile || !rvs || !rfs)
		return 0;

	prog = 0;
	vsrc = NULL;
	fsrc = NULL;

	vsrc = karinLoadSource(vfile);
	if(!vsrc)
		return 0;

	vshader = karinLoadShader(GL_VERTEX_SHADER, vsrc);
	if(glIsShader(vshader))
	{
		fsrc = karinLoadSource(ffile);
		if(fsrc)
		{
			fshader = karinLoadShader(GL_FRAGMENT_SHADER, fsrc);
			if(glIsShader(fshader))
			{
				prog = glCreateProgram();
				if(glIsProgram(prog))
				{
					if(karinLinkShaderToProgram(prog, vshader, fshader))
					{
						*rvs = vshader;
						*rfs = fshader;
					}
				}
			}
		}
	}

	free(vsrc);
	free(fsrc);

	if(prog == 0)
	{
		if(glIsShader(vshader))
			glDeleteShader(vshader);
		if(glIsShader(fshader))
			glDeleteShader(fshader);
		if(glIsProgram(prog))
			glDeleteProgram(prog);
	}
	return prog;
}

void karinSetupMatrix()
{
	if(!prog_inited)
		return;

	Mesa_matmul4(projection_matrix, modelview_matrix, mvp_matrix);
	glUniformMatrix4fv(locations[U_MODELVIEWPROJECTIONMATRIX], 1, GL_FALSE, mvp_matrix);
}

void karinSetupAlpha(GLclampf f)
{
	if(!prog_inited)
		return;

	glUniform1f(locations[U_ALPHA], f);
}

void karinSetupTexture(GLuint t)
{
	if(!prog_inited)
		return;

	glUniform1i(locations[U_FRAGMENTMAP0], t);
}

void karinSetupVertex(const GLfloat *ptr)
{
	if(!prog_inited)
		return;

	glVertexAttribPointer(locations[ATTR_VERTEX], 2, GL_FLOAT, GL_FALSE, 0, ptr);
}

void karinSetupTexCoord(const GLfloat *ptr)
{
	if(!prog_inited)
		return;

	glVertexAttribPointer(locations[ATTR_TEXCOORD], 2, GL_FLOAT, GL_FALSE, 0, ptr);
}

void karinBeginRender2D(int width, int height)
{
	if(!prog_inited)
		return;

	glGetIntegerv(GL_CURRENT_PROGRAM, &current_attrib.program);
	glUseProgram(program);
	glEnableVertexAttribArray(locations[ATTR_VERTEX]);
	glEnableVertexAttribArray(locations[ATTR_TEXCOORD]);
	karinSetupState();
	glUniform1i(locations[U_FRAGMENTMAP0], 0);

	// Matrix
	glViewport(0, 0, width, height);

	Mesa_glLoadIdentity(projection_matrix);
	Mesa_glOrtho(NULL, 0.0, width, 0.0, height, -1.0, 1.0, projection_matrix);
}

void karinEndRender2D()
{
	if(!prog_inited)
		return;

	glDisableVertexAttribArray(locations[ATTR_VERTEX]);
	glDisableVertexAttribArray(locations[ATTR_TEXCOORD]);
	karinRestoreState();
	glUseProgram(current_attrib.program);
}

void karinInit()
{
	int i;

	if(prog_inited)
		return;

	program = karinLoadProgram(VSHADER_FILE, FSHADER_FILE, &vshader, &fshader);
	if(glIsProgram(program))
	{
		for(i = 0; i < TOTAL_LOCATION; i++)
		{
			if(Location_Names[i][0] == 'u')
				locations[i] = glGetUniformLocation(program, Location_Names[i]);
			else
				locations[i] = glGetAttribLocation(program, Location_Names[i]);
		}
		prog_inited = 1;
		//printf("[Harmattan]: Init vkb renderer shader program.\n");
	}
	else
	{
		prog_inited = 0;
		//printf("[Harmattan]: Init vkb renderer shader program fail.\n");
	}
}

void karinDestroy()
{
	if(!prog_inited)
		return;

	glDetachShader(program, vshader);
	glDetachShader(program, fshader);
	glDeleteShader(vshader);
	glDeleteShader(fshader);
	glDeleteProgram(program);

	vshader = 0;
	fshader = 0;
	program = 0;
	memset(locations, 0, sizeof(locations));

	prog_inited = 0;
}



/* matrix */

#define A(row,col)  a[(col<<2)+row]
#define B(row,col)  b[(col<<2)+row]
#define P(row,col)  product[(col<<2)+row]
void Mesa_matmul4(const float a[16], const float b[16], float product[16])
{
	int i;

	if(!a || !b || !product)
		return;

	for (i = 0; i < 4; i++) {
		const float ai0=A(i,0),  ai1=A(i,1),  ai2=A(i,2),  ai3=A(i,3);
		P(i,0) = ai0 * B(0,0) + ai1 * B(1,0) + ai2 * B(2,0) + ai3 * B(3,0);
		P(i,1) = ai0 * B(0,1) + ai1 * B(1,1) + ai2 * B(2,1) + ai3 * B(3,1);
		P(i,2) = ai0 * B(0,2) + ai1 * B(1,2) + ai2 * B(2,2) + ai3 * B(3,2);
		P(i,3) = ai0 * B(0,3) + ai1 * B(1,3) + ai2 * B(2,3) + ai3 * B(3,3);
	}
}
#undef A
#undef B
#undef P

void Mesa_glLoadIdentity(float m[16])
{
	if(!m)
		return;
	static float Identity[16] = {
		1.0, 0.0, 0.0, 0.0,
		0.0, 1.0, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		0.0, 0.0, 0.0, 1.0
	};
	memcpy(m, Identity, sizeof(float)*16);
}

void Mesa_glRotate(const float mat[16], float angle, float x, float y, float z, float r[16])
{
#define SQRTF(X)  (float) sqrt((float) (X))
#define C_DEG2RAD (M_PI/180.0)

	if(!mat || !r)
		return;

	float xx, yy, zz, xy, yz, zx, xs, ys, zs, one_c, s, c;
	float m[16];
	bool optimized;

	s = (float) sin( angle * C_DEG2RAD );
	c = (float) cos( angle * C_DEG2RAD );

	Mesa_glLoadIdentity(m);
	optimized = false;

#define M(row,col)  m[col*4+row]

	if (x == 0.0F) {
		if (y == 0.0F) {
			if (z != 0.0F) {
				optimized = true;
				/* rotate only around z-axis */
				M(0,0) = c;
				M(1,1) = c;
				if (z < 0.0F) {
					M(0,1) = s;
					M(1,0) = -s;
				}
				else {
					M(0,1) = -s;
					M(1,0) = s;
				}
			}
		}
		else if (z == 0.0F) {
			optimized = true;
			/* rotate only around y-axis */
			M(0,0) = c;
			M(2,2) = c;
			if (y < 0.0F) {
				M(0,2) = -s;
				M(2,0) = s;
			}
			else {
				M(0,2) = s;
				M(2,0) = -s;
			}
		}
	}
	else if (y == 0.0F) {
		if (z == 0.0F) {
			optimized = true;
			/* rotate only around x-axis */
			M(1,1) = c;
			M(2,2) = c;
			if (x < 0.0F) {
				M(1,2) = s;
				M(2,1) = -s;
			}
			else {
				M(1,2) = -s;
				M(2,1) = s;
			}
		}
	}

	if (!optimized) {
		const float mag = SQRTF(x * x + y * y + z * z);

		if (mag <= 1.0e-4) {
			/* no rotation, leave mat as-is */
			return;
		}

		x /= mag;
		y /= mag;
		z /= mag;

		xx = x * x;
		yy = y * y;
		zz = z * z;
		xy = x * y;
		yz = y * z;
		zx = z * x;
		xs = x * s;
		ys = y * s;
		zs = z * s;
		one_c = 1.0F - c;

		/* We already hold the identity-matrix so we can skip some statements */
		M(0,0) = (one_c * xx) + c;
		M(0,1) = (one_c * xy) - zs;
		M(0,2) = (one_c * zx) + ys;
		/*    M(0,3) = 0.0F; */

		M(1,0) = (one_c * xy) + zs;
		M(1,1) = (one_c * yy) + c;
		M(1,2) = (one_c * yz) - xs;
		/*    M(1,3) = 0.0F; */

		M(2,0) = (one_c * zx) - ys;
		M(2,1) = (one_c * yz) + xs;
		M(2,2) = (one_c * zz) + c;
		/*    M(2,3) = 0.0F; */

		/*
		 *       M(3,0) = 0.0F;
		 *             M(3,1) = 0.0F;
		 *                   M(3,2) = 0.0F;
		 *                         M(3,3) = 1.0F;
		 *                         */
	}
#undef M

	Mesa_matmul4(mat, m, r);
}

void Mesa_glTranslate(const float mat[16], float x, float y, float z, float m[16])
{
	if(!mat || !m)
		return;
	if(mat != m)
		memcpy(m, mat, sizeof(float) * 16);
	m[12] = m[0] * x + m[4] * y + m[8]  * z + m[12];
	m[13] = m[1] * x + m[5] * y + m[9]  * z + m[13];
	m[14] = m[2] * x + m[6] * y + m[10] * z + m[14];
	m[15] = m[3] * x + m[7] * y + m[11] * z + m[15];
}

void Mesa_glOrtho(const float mat[16], float left, float right, float bottom, float top, float nearval, float farval, float r[16])
{
	if(!r)
		return;

	if (left == right ||
			bottom == top ||
			nearval == farval)
	{
		//_mesa_error( ctx,  GL_INVALID_VALUE, "glOrtho" );
		return;
	}
	float m[16];

#define M(row,col)  m[col*4+row]
	M(0,0) = 2.0F / (right-left);
	M(0,1) = 0.0F;
	M(0,2) = 0.0F;
	M(0,3) = -(right+left) / (right-left);

	M(1,0) = 0.0F;
	M(1,1) = 2.0F / (top-bottom);
	M(1,2) = 0.0F;
	M(1,3) = -(top+bottom) / (top-bottom);

	M(2,0) = 0.0F;
	M(2,1) = 0.0F;
	M(2,2) = -2.0F / (farval-nearval);
	M(2,3) = -(farval+nearval) / (farval-nearval);

	M(3,0) = 0.0F;
	M(3,1) = 0.0F;
	M(3,2) = 0.0F;
	M(3,3) = 1.0F;
#undef M

	if(mat)
		Mesa_matmul4(mat, m, r);
	else
		memcpy(r, m, sizeof(float) * 16);
}
