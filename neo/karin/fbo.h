#ifndef _KARIN_FBO_H
#define _KARIN_FBO_H

#include <GLES2/gl2.h>

class karin_FrameBufferObject
{
	public:
		karin_FrameBufferObject(GLuint w, GLuint h, GLuint vw, GLuint vh, GLboolean create = GL_FALSE);
		virtual ~karin_FrameBufferObject();

	public:
		GLuint texture() const { return m_texture; }
		void bind();
		void unbind();
		unsigned isBind() const;
		unsigned inited() const { return m_inited; }
		GLuint handle() const {return m_handle; }
		GLuint width() const { return m_width; }
		GLuint height() const { return m_height; };
		GLuint viewportWidth() const { return m_viewportWidth; }
		GLuint viewportHeight() const { return m_viewportHeight; };
		void blit();
		GLenum textureFormat() const { return M_TextureFormat; }
		void resize(GLuint w, GLuint h, GLuint vw, GLuint vh);

	private:
		unsigned init(GLuint width, GLuint height);
		void destory();
		void initBuffer(GLfloat w, GLfloat h);
		void attachFrameBuffer(GLuint w, GLuint h);
		unsigned getError() const;
		unsigned getFBOStatus() const;
		void printRenderBufferParams() const;
		GLuint genTexture2D(GLuint width, GLuint height);
		karin_FrameBufferObject(const karin_FrameBufferObject &);
		karin_FrameBufferObject & operator= (const karin_FrameBufferObject &);
		enum{
			Depth_Render_Buffer = 0,
			Stencil_Render_Buffer,
			Total_Render_Buffer,
		};
		enum{
			Vertex_Buffer = 0,
			TexCoord_Buffer,
			Total_Buffer,
		};

	private:
		unsigned m_inited;
		GLuint m_handle;
		GLuint m_texture;
		GLuint m_renderBuffer[Total_Render_Buffer];
		GLuint m_viewportWidth;
		GLuint m_viewportHeight;
		GLuint m_width;
		GLuint m_height;
		GLuint m_buffer[Total_Buffer];
		static const GLuint M_MinSize = 2;
		static const GLenum M_TextureFormat = GL_RGBA;
};

#endif
