#ifndef AUDIT_DC_RLGL_GL_H
#define AUDIT_DC_RLGL_GL_H

typedef unsigned int GLenum;
typedef int GLsizei;
typedef unsigned char GLboolean;

#define GL_TEXTURE_2D 0x0DE1u
#define GL_VERTEX_ARRAY 0x8074u
#define GL_NORMAL_ARRAY 0x8075u
#define GL_COLOR_ARRAY 0x8076u
#define GL_TEXTURE_COORD_ARRAY 0x8078u
#define GL_FLOAT 0x1406u
#define GL_UNSIGNED_BYTE 0x1401u
#define GL_BGRA 0x80E1u
#define GL_TRIANGLES 0x0004u
#define GL_QUADS 0x0007u

void glEnable(GLenum cap);
void glDisable(GLenum cap);
void glBindTexture(GLenum target, unsigned int texture);
void glEnableClientState(GLenum cap);
void glDisableClientState(GLenum cap);
void glVertexPointer(int size, GLenum type, GLsizei stride, const void *pointer);
void glTexCoordPointer(int size, GLenum type, GLsizei stride, const void *pointer);
void glColorPointer(int size, GLenum type, GLsizei stride, const void *pointer);
void glNormalPointer(GLenum type, GLsizei stride, const void *pointer);
void glDrawArrays(GLenum mode, int first, GLsizei count);
void glNormal3f(float x, float y, float z);

#endif
