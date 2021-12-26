#include "../../3rdparty-deps/libretro.h"
#include "gl_render.h"
#include "../../3rdparty-deps/glad.h"
#include <math.h>
#include <windows.h>
#include <stdbool.h>
video g_video;

static const PIXELFORMATDESCRIPTOR pfd = {
	sizeof(PIXELFORMATDESCRIPTOR),1,
	PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
	PFD_TYPE_RGBA,32,0,0,0,0,0,0,0,0,0,0,0,0,0,32,0,0,PFD_MAIN_PLANE,
	0,0,0,0 };

static struct {
	GLuint vao;
	GLuint vbo;
	GLuint program;
	GLuint fshader;
	GLuint vshader;
	GLint i_pos;
	GLint i_coord;
	GLint u_tex;
	GLint u_mvp;
} g_shader = { 0 };

static float g_scale = 2;
static bool g_win = false;

static const char* g_vshader_src =R"(
#version 330
in vec2 i_pos;
in vec2 i_coord;
out vec2 o_coord;
uniform mat4 u_mvp;
void main() {
o_coord = i_coord;
gl_Position = vec4(i_pos, 0.0, 1.0) * u_mvp;
})";

static const char* g_fshader_src =R"(
#version 330
in vec2 o_coord;
uniform sampler2D u_tex;
void main() {
gl_FragColor = texture2D(u_tex, o_coord);
})";

uintptr_t core_get_current_framebuffer() { return g_video.fbo_id; }

void ortho2d(float m[4][4], float left, float right, float bottom, float top) {
	m[3][3] = 1;
	m[0][0] = 2.0f / (right - left);
	m[1][1] = 2.0f / (top - bottom);
	m[2][2] = -1.0f;
	m[3][0] = -(right + left) / (right - left);
	m[3][1] = -(top + bottom) / (top - bottom);
}

GLuint compile_shader(unsigned type, unsigned count, const char** strings) {
	GLint status;
	GLuint shader = glCreateShaderProgramv(type, 1, strings);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE) {
		char buffer[4096]={0};
		glGetShaderInfoLog(shader, sizeof(buffer), NULL, buffer);
	}
	return shader;
}

void init_shaders() {
	GLint status;
	g_shader.vshader = compile_shader(GL_VERTEX_SHADER, 1, &g_vshader_src);
	g_shader.fshader = compile_shader(GL_FRAGMENT_SHADER, 1, &g_fshader_src);

	glGenProgramPipelines(1, &g_shader.program);
	glBindProgramPipeline(g_shader.program);
	glUseProgramStages(g_shader.program, GL_VERTEX_SHADER_BIT, g_shader.vshader);
	glUseProgramStages(g_shader.program, GL_FRAGMENT_SHADER_BIT,
		g_shader.fshader);

	glGetProgramiv(g_shader.program, GL_LINK_STATUS, &status);
	if (status == GL_FALSE) {
		char buffer[4096] = { 0 };
		glGetProgramInfoLog(g_shader.program, sizeof(buffer), NULL, buffer);
	}
	g_shader.i_pos = glGetAttribLocation(g_shader.vshader, "i_pos");
	g_shader.i_coord = glGetAttribLocation(g_shader.vshader, "i_coord");
	g_shader.u_tex = glGetUniformLocation(g_shader.fshader, "u_tex");
	g_shader.u_mvp = glGetUniformLocation(g_shader.vshader, "u_mvp");

	glGenVertexArrays(1, &g_shader.vao);
	glGenBuffers(1, &g_shader.vbo);
	glProgramUniform1i(g_shader.fshader, g_shader.u_tex, 0);

	float m[4][4] = { 0 };
	if (g_video.hw.bottom_left_origin)
		ortho2d(m, -1, 1, 1, -1);
	else
		ortho2d(m, -1, 1, -1, 1);

	glProgramUniformMatrix4fv(g_shader.vshader, g_shader.u_mvp, 1, GL_FALSE,
		(float*)m);
	glBindProgramPipeline(0);
}

void refresh_vertex_data() {

	float bottom = (float)g_video.base_h / g_video.tex_h;
	float right = (float)g_video.base_w / g_video.tex_w;

	typedef struct {
		float x;
		float y;
		float s;
		float t;
	} vertex_data;

	vertex_data vert[] = {
		// pos, coord
		-1.0f, -1.0f,  0.0f,  bottom, // left-bottom
		-1.0f, 1.0f,  0.0f,  0.0f,   // left-top
		1.0f,  -1.0f, right, bottom, // right-bottom
		1.0f,  1.0f, right, 0.0f,   // right-top
	};

	glBindVertexArray(g_shader.vao);
	glBindBuffer(GL_ARRAY_BUFFER, g_shader.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data) * 4, vert, GL_STATIC_DRAW);
	glEnableVertexAttribArray(g_shader.i_pos);
	glEnableVertexAttribArray(g_shader.i_coord);
	glVertexAttribPointer(g_shader.i_pos, 2, GL_FLOAT, GL_FALSE,
		sizeof(vertex_data), (void*)offsetof(vertex_data, x));
	glVertexAttribPointer(g_shader.i_coord, 2, GL_FLOAT, GL_FALSE,
		sizeof(vertex_data), (void*)offsetof(vertex_data, s));
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void init_framebuffer(int width, int height) {
	if (g_video.fbo_id)
		glDeleteFramebuffers(1, &g_video.fbo_id);
	if (g_video.rbo_id)
		glDeleteRenderbuffers(1, &g_video.rbo_id);

	glGenFramebuffers(1, &g_video.fbo_id);
	glBindFramebuffer(GL_FRAMEBUFFER, g_video.fbo_id);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
		g_video.tex_id, 0);
	if (g_video.hw.depth) {
		glGenRenderbuffers(1, &g_video.rbo_id);
		glBindRenderbuffer(GL_RENDERBUFFER, g_video.rbo_id);
		glRenderbufferStorage(GL_RENDERBUFFER,
			g_video.hw.stencil ? GL_DEPTH24_STENCIL8
			: GL_DEPTH_COMPONENT24,
			width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER,
			g_video.hw.stencil ? GL_DEPTH_STENCIL_ATTACHMENT
			: GL_DEPTH_ATTACHMENT,
			GL_RENDERBUFFER, g_video.rbo_id);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
	}
	glCheckFramebufferStatus(GL_FRAMEBUFFER);
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void resize_cb() {
	RECT rc;
	GetClientRect(g_video.hwnd, &rc);
	unsigned width = rc.right - rc.left;
	unsigned height = rc.bottom - rc.top;
	int pad_x = 0, pad_y = 0;
	if (!width || !height)
		return;
	float screenaspect = width / height;
	unsigned base_h = g_video.base_h;
	if (base_h == 0)base_h = 1;
	unsigned base_w = (unsigned)roundf(base_h * screenaspect);
	if (width >= base_w && height >= base_h)
	{
		unsigned scale = min(width / base_w, height / base_h);
		pad_x = width - base_w * scale;
		pad_y = height - base_h * scale;
	}
	width -= pad_x;
	height -= pad_y;
	glViewport(pad_x / 2, pad_y / 2, width, height);
}

void create_window(int width, int height, HWND hwnd) {

	g_video.hwnd = hwnd;
	g_video.hDC = GetDC(hwnd);
	unsigned int PixelFormat;
	PixelFormat = ChoosePixelFormat(g_video.hDC, &pfd);
	SetPixelFormat(g_video.hDC, PixelFormat, &pfd);
	if (!(g_video.hRC = wglCreateContext(g_video.hDC)))
		return;
	if (!wglMakeCurrent(g_video.hDC, g_video.hRC))
		return;

	gladLoadGL();
	init_shaders();

	typedef bool(APIENTRY* PFNWGLSWAPINTERVALFARPROC)(int);
	PFNWGLSWAPINTERVALFARPROC wglSwapIntervalEXT = 0;
	wglSwapIntervalEXT =
		(PFNWGLSWAPINTERVALFARPROC)wglGetProcAddress("wglSwapIntervalEXT");
	if (wglSwapIntervalEXT)
		wglSwapIntervalEXT(1);
	g_win = true;
	g_video.last_w = 0;
	g_video.last_h = 0;
}

void resize_to_aspect(double ratio, int sw, int sh, int* dw, int* dh) {
	*dw = sw;
	*dh = sh;

	if (ratio <= 0)
		ratio = (double)sw / sh;

	if ((float)sw / sh < 1)
		*dw = *dh * ratio;
	else
		*dh = *dw / ratio;
}

BOOL CenterWindow(HWND wind) {
	HWND parent;
	RECT rectw, rectp;

	// make the window relative to its parent
	if ((parent = GetParent(wind)) != NULL) {
		GetWindowRect(wind, &rectw);
		GetWindowRect(parent, &rectp);

		unsigned width = rectw.right - rectw.left;
		unsigned height = rectw.bottom - rectw.top;

		unsigned x =
			((rectp.right - rectp.left) - width) / 2 + rectp.left;
		unsigned y =
			((rectp.bottom - rectp.top) - height) / 2 + rectp.top;

		unsigned screenw = GetSystemMetrics(SM_CXSCREEN);
		unsigned screenh = GetSystemMetrics(SM_CYSCREEN);

		// make sure that the dialog box never moves outside of the screen
		if (x < 0)
			x = 0;
		if (y < 0)
			y = 0;
		if (x + width > screenw)
			x = screenw - width;
		if (y + height > screenh)
			y = screenh - height;

		MoveWindow(wind, x, y, width, height, FALSE);

		return TRUE;
	}

	return FALSE;
}

void ResizeWindow(HWND hWnd, int nWidth, int nHeight) {
	RECT client, wind;
	POINT diff;
	GetClientRect(hWnd, &client);
	GetWindowRect(hWnd, &wind);
	diff.x = (wind.right - wind.left) - client.right;
	diff.y = (wind.bottom - wind.top) - client.bottom;
	MoveWindow(hWnd, wind.left, wind.top, nWidth + diff.x,
		nHeight + diff.y, TRUE);
}

void video_init(const struct retro_game_geometry* geom, HWND hwnd) {
	int width = 0, height = 0;

	resize_to_aspect(geom->aspect_ratio, geom->base_width * 1,
		geom->base_height * 1, &width, &height);

	g_video.software_rast = g_video.hw.context_reset;

	width *= 1;
	height *= 1;

	create_window(width, height, hwnd);

	if (g_video.tex_id)
		glDeleteTextures(1, &g_video.tex_id);

	g_video.tex_id = 0;

	if (!g_video.pixfmt)
		video_set_pixel_format(RETRO_PIXEL_FORMAT_0RGB1555);

	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	CenterWindow(hwnd);

	glGenTextures(1, &g_video.tex_id);

	g_video.pitch = geom->max_width * g_video.bpp;

	glBindTexture(GL_TEXTURE_2D, g_video.tex_id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, geom->max_width, geom->max_height, 0,
		g_video.pixtype, g_video.pixfmt, NULL);

	glBindTexture(GL_TEXTURE_2D, 0);

	init_framebuffer(geom->max_width, geom->max_height);

	g_video.tex_w = geom->max_width;
	g_video.tex_h = geom->max_height;
	g_video.base_w = geom->base_width;
	g_video.base_h = geom->base_height;
	g_video.aspect = geom->aspect_ratio;

	refresh_vertex_data();
	if (g_video.hw.context_reset)
		g_video.hw.context_reset();

	int nScreenWidth = GetSystemMetrics(SM_CXSCREEN);
	int nScreenHeight = GetSystemMetrics(SM_CYSCREEN);
	double x = (double)nScreenWidth / (double)geom->base_width;
	double y = (double)nScreenHeight / (double)geom->base_height;
	double factor = x < y ? x : y;
	int int_factor = unsigned(factor);

	int nominal = int_factor;
	ResizeWindow(g_video.hwnd, g_video.base_w * nominal,
		g_video.base_h * nominal);
}

bool video_set_pixel_format(unsigned format) {
	switch (format) {
	case RETRO_PIXEL_FORMAT_0RGB1555:
		g_video.pixfmt = GL_UNSIGNED_SHORT_5_5_5_1;
		g_video.pixtype = GL_BGRA;
		g_video.bpp = sizeof(uint16_t);
		break;
	case RETRO_PIXEL_FORMAT_XRGB8888:
		g_video.pixfmt = GL_UNSIGNED_INT_8_8_8_8_REV;
		g_video.pixtype = GL_BGRA;
		g_video.bpp = sizeof(uint32_t);
		break;
	case RETRO_PIXEL_FORMAT_RGB565:
		g_video.pixfmt = GL_UNSIGNED_SHORT_5_6_5;
		g_video.pixtype = GL_RGB;
		g_video.bpp = sizeof(uint16_t);
		break;
	default:
		g_video.pixfmt = GL_UNSIGNED_SHORT_5_5_5_1;
		g_video.pixtype = GL_BGRA;
		g_video.bpp = sizeof(uint16_t);
		break;
	}

	return true;
}

void video_refresh(const void* data, unsigned width, unsigned height,
	unsigned pitch) {
	if (data == NULL)
		return;
	if (g_video.base_w != width || g_video.base_h != height) {
		g_video.base_h = height;
		g_video.base_w = width;

		refresh_vertex_data();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	resize_cb();
	glBindTexture(GL_TEXTURE_2D, g_video.tex_id);

	

	if (data && data != RETRO_HW_FRAME_BUFFER_VALID) {
		if (pitch != g_video.pitch)
			g_video.pitch = pitch;
		glPixelStorei(GL_UNPACK_ROW_LENGTH, g_video.pitch / g_video.bpp);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, g_video.pixtype,
			g_video.pixfmt, data);
	}

	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(0);
	glBindProgramPipeline(g_shader.program);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, g_video.tex_id);
	glProgramUniform1i(g_shader.fshader, g_shader.u_tex, 0);
	glBindVertexArray(g_shader.vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
	glBindProgramPipeline(0);
	SwapBuffers(g_video.hDC);
}

void video_deinit() {
	if (g_video.tex_id) {
		glDeleteTextures(1, &g_video.tex_id);
		g_video.tex_id = 0;
	}
	if (g_video.fbo_id) {
		glDeleteFramebuffers(1, &g_video.fbo_id);
		g_video.fbo_id = 0;
	}

	if (g_video.rbo_id) {
		glDeleteRenderbuffers(1, &g_video.rbo_id);
		g_video.rbo_id = 0;
	}
	glDisableVertexAttribArray(1);
	glDeleteBuffers(1, &g_shader.vbo);
	glDeleteVertexArrays(1, &g_shader.vbo);

	glDeleteProgramPipelines(1, &g_shader.program);
	glDeleteShader(g_shader.fshader);
	glDeleteShader(g_shader.vshader);

	if (g_video.hRC) {
		wglMakeCurrent(0, 0);
		wglDeleteContext(g_video.hRC);
	}

	if (g_video.hDC)
		ReleaseDC(g_video.hwnd, g_video.hDC);
	close_gl();
}