#include "fbo.h"

#include <stdio.h>
#include <string.h>
#include <GLES2/gl2ext.h>

#include "gl2_abi.h"
#include "gl_vkb.h"

karin_FrameBufferObject::karin_FrameBufferObject(GLuint width, GLuint height, GLuint vw, GLuint vh, GLboolean create)
	: m_inited(0),
	m_handle(0),
	m_texture(0),
	m_viewportWidth(vw <= 0 ? karin_FrameBufferObject::M_MinSize : vw),
	m_viewportHeight(vh <= 0 ? karin_FrameBufferObject::M_MinSize : vh),
	m_width(width <= 0 ? karin_FrameBufferObject::M_MinSize : width),
	m_height(height <= 0 ? karin_FrameBufferObject::M_MinSize : height)
{
	memset(m_renderBuffer, 0, karin_FrameBufferObject::Total_Render_Buffer * sizeof(GLuint));
	memset(m_buffer, 0, karin_FrameBufferObject::Total_Buffer * sizeof(GLuint));
	if(create)
	{
		m_inited = init(m_width, m_height);
		initBuffer(m_viewportWidth, m_viewportHeight);
	}
}

karin_FrameBufferObject::~karin_FrameBufferObject()
{
	destory();
}

unsigned karin_FrameBufferObject::init(GLuint w, GLuint h)
{
	GLenum status;

	if(m_inited)
		return 1;

	glGenFramebuffers(1, &m_handle);
	glBindFramebuffer(GL_FRAMEBUFFER, m_handle);

	attachFrameBuffer(m_width, m_height);

	status = getFBOStatus();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return ((status == GL_FRAMEBUFFER_COMPLETE) ? 1 : 0);
}

void karin_FrameBufferObject::destory()
{
	if(m_inited)
		return;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteTextures(1, &m_texture);
	glDeleteRenderbuffers(Total_Render_Buffer, m_renderBuffer);
	glDeleteFramebuffers(1, &m_handle);
	glDeleteBuffers(karin_FrameBufferObject::Total_Buffer, m_buffer);
	m_inited = 0;
}

unsigned karin_FrameBufferObject::getFBOStatus() const
{
#define K_CASE(x) case x: printf("[karin_FrameBufferObject]: FBO Status->%s\n", #x); return x;
	GLenum status;

	status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	switch(status)
	{
		K_CASE(GL_FRAMEBUFFER_COMPLETE)
			K_CASE(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT)
			K_CASE(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT)
			K_CASE(GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS)
			K_CASE(GL_FRAMEBUFFER_UNSUPPORTED)
	}
#undef K_CASE
}

unsigned karin_FrameBufferObject::getError() const
{
	GLuint error;

	static const char *ErrorString[] = {
		"GL_INVALID_ENUM", 
		"GL_INVALID_VALUE", 
		"GL_INVALID_OPERATION", 
		"GL_OUT_OF_MEMORY", 
	};
	error = glGetError();
	if(error == GL_NO_ERROR)
	{
		fprintf(stdout, "[karin_FrameBufferObject]: GL NoErr\n");
		return 0;
	}

	fprintf(stderr, "[karin_FrameBufferObject]: GL Error->%s\n", ErrorString[error - 0x500]);
	return 1;
}

GLuint karin_FrameBufferObject::genTexture2D(GLuint w, GLuint h)
{
	GLuint tex;

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexImage2D(GL_TEXTURE_2D, 0, textureFormat(), w, h, 0, textureFormat(), GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	return tex;
}

void karin_FrameBufferObject::bind()
{
	if(!m_inited)
		return;

	glBindFramebuffer(GL_FRAMEBUFFER, m_handle);
}

void karin_FrameBufferObject::unbind()
{
	if(!m_inited)
		return;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void karin_FrameBufferObject::blit()
{
	/*
	glBindTexture(GL_TEXTURE_2D, m_texture);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, m_width, m_height);
	*/
	unbind();
	karinBeginRender2D(m_viewportWidth, m_viewportHeight);
	{
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_BLEND);
		karinSetupAlpha(1.0);
		if(m_inited)
		{
			glBindTexture(GL_TEXTURE_2D, m_texture);
			glBindBuffer(GL_ARRAY_BUFFER, m_buffer[karin_FrameBufferObject::TexCoord_Buffer]);
			karinSetupTexCoord(NULL);
			glBindBuffer(GL_ARRAY_BUFFER, m_buffer[karin_FrameBufferObject::Vertex_Buffer]);
			karinSetupVertex(NULL);
			karinSetupTexture(0);
			karinSetupMatrix();
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
			glBindTexture(GL_TEXTURE_2D, 0);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}

		if(h_cUsingVKB.GetBool())
			karinRenderVKBToFBO();

		glFlush();
	}
	karinEndRender2D();
}

void karin_FrameBufferObject::initBuffer(GLfloat w, GLfloat h)
{
	if(!m_inited)
		return;

	const GLfloat vs[] = {
		0.0, 0.0,
		w, 0.0,
		w, h,
		0.0, h,
	};
	const GLfloat ts[] = {
		0.0, 0.0,
		1.0, 0.0,
		1.0, 1.0,
		0.0, 1.0,
	};

	glGenBuffers(karin_FrameBufferObject::Total_Buffer, m_buffer);

	glBindBuffer(GL_ARRAY_BUFFER, m_buffer[karin_FrameBufferObject::Vertex_Buffer]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vs), vs, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ARRAY_BUFFER, m_buffer[karin_FrameBufferObject::TexCoord_Buffer]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(ts), ts, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void karin_FrameBufferObject::resize(GLuint w, GLuint h, GLuint vw, GLuint vh)
{
	if(!m_inited)
		return;

	if((w != m_width || h != m_height) && w > karin_FrameBufferObject::M_MinSize && h > karin_FrameBufferObject::M_MinSize)
	{
		m_width = w;
		m_height = h;

		glBindFramebuffer(GL_FRAMEBUFFER, m_handle);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
		glDeleteTextures(1, &m_texture);
		m_texture = 0;

		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_OES, GL_RENDERBUFFER, 0);
		glDeleteRenderbuffers(karin_FrameBufferObject::Total_Render_Buffer, m_renderBuffer);
		memset(m_renderBuffer, 0, karin_FrameBufferObject::Total_Render_Buffer * sizeof(GLuint));
		memset(m_buffer, 0, karin_FrameBufferObject::Total_Buffer * sizeof(GLuint));

		attachFrameBuffer(m_width, m_height);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	if((vw != m_viewportWidth || vh != m_viewportHeight) && vw > karin_FrameBufferObject::M_MinSize && vh > karin_FrameBufferObject::M_MinSize)
	{
		m_viewportWidth = vw;
		m_viewportHeight = vh;

		const GLfloat vs[] = {
			0.0, 0.0,
			m_viewportWidth, 0.0,
			m_viewportWidth, m_viewportHeight,
			0.0, m_viewportHeight,
		};

		glBindBuffer(GL_ARRAY_BUFFER, m_buffer[karin_FrameBufferObject::Vertex_Buffer]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vs), vs);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
}

void karin_FrameBufferObject::attachFrameBuffer(GLuint w, GLuint h)
{
	GLenum status;

	m_texture = genTexture2D(w, h);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture, 0);

	glGenRenderbuffers(karin_FrameBufferObject::Total_Render_Buffer, m_renderBuffer);

	glBindRenderbuffer(GL_RENDERBUFFER, m_renderBuffer[karin_FrameBufferObject::Depth_Render_Buffer]);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, w, h);
	getError();
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24_OES, w, h);
	getError();

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_renderBuffer[karin_FrameBufferObject::Depth_Render_Buffer]);
	getError();
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_renderBuffer[karin_FrameBufferObject::Depth_Render_Buffer]);
	getError();

	status = getFBOStatus();
	printRenderBufferParams();
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

unsigned karin_FrameBufferObject::isBind() const
{
	GLint cur_fbo;

	if(!m_inited)
		return 0;

	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &cur_fbo);

	return cur_fbo == m_handle ? 1 : 0;
}

void karin_FrameBufferObject::printRenderBufferParams() const
{
	GLint _v;
#define _GETP(x) glGetRenderbufferParameteriv(GL_RENDERBUFFER, x, &_v); \
	printf(#x" -> %d 0x%x\n", _v, _v);
	_GETP(GL_RENDERBUFFER_WIDTH)
		_GETP(GL_RENDERBUFFER_HEIGHT)
		_GETP(GL_RENDERBUFFER_INTERNAL_FORMAT)
		_GETP(GL_RENDERBUFFER_RED_SIZE)
		_GETP(GL_RENDERBUFFER_GREEN_SIZE)
		_GETP(GL_RENDERBUFFER_BLUE_SIZE)
		_GETP(GL_RENDERBUFFER_ALPHA_SIZE)
		_GETP(GL_RENDERBUFFER_DEPTH_SIZE)
		_GETP(GL_RENDERBUFFER_STENCIL_SIZE)
#undef _GETP
}
