#include <print>
#include <string>
#include <ranges>
#include <cstdio>
#include <cstring>

#include <glad/glad.h>
#include <GL/osmesa.h>
#include <png.h>

#define iassert(expr) if (!(expr)) throw std::format("Assertion `{}` failed", #expr)

using namespace std::literals::string_literals;

class Container
{
private:
	GLuint shape_vbo, shape2_vbo, shape_vao, shape2_vao;
	GLuint vs, fs, fs2, shader, shader2;

public:
	void init_user()
	{
		// Shaders
		const char* vs_code {
			R"(#version 400
			in vec3 vertex;
			in float offset;
			void main() {
				gl_Position = vec4(vertex + offset, 1.0);
			})"
		};

		const char* fs_code {
			R"(#version 400
			out vec4 color;
			void main() {
				color = vec4(0.5, 0.0, 0.5, 1.0);
			})"
		};

		vs = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vs, 1, &vs_code, nullptr);
		glCompileShader(vs);

		fs = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fs, 1, &fs_code, nullptr);
		glCompileShader(fs);

		shader = glCreateProgram();
		glAttachShader(shader, vs);
		glAttachShader(shader, fs);
		glLinkProgram(shader);

		const char* fs2_code {
			R"(#version 400
			out vec4 color;
			void main() {
				color = vec4(0.5, 0.5, 0.0, 1.0);
			})"
		};

		fs2 = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fs2, 1, &fs2_code, nullptr);
		glCompileShader(fs2);

		shader2 = glCreateProgram();
		glAttachShader(shader2, vs);
		glAttachShader(shader2, fs2);
		glLinkProgram(shader2);

		// Vertices
		// Shape 1
		const float vertices[] {
			-0.5, 0.5, 0,
			0.5, -0.5, 0,
			-0.5, -0.5, 0,

			0.5, 0.5, 0,
			0.5, -0.5, 0,
			-0.5, 0.5, 0
		};

		const float offsets[] {
			0.25, 0.25, 0.25,
			0.25, 0.25, 0.25
		};

		glGenBuffers(1, &shape_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, shape_vbo);
		glNamedBufferStorage(shape_vbo, sizeof(vertices) + sizeof(offsets), nullptr, GL_DYNAMIC_STORAGE_BIT);
		glNamedBufferSubData(shape_vbo, 0, sizeof(vertices), vertices);
		glNamedBufferSubData(shape_vbo, sizeof(vertices), sizeof(offsets), offsets);

		glGenVertexArrays(1, &shape_vao);
		glBindVertexArray(shape_vao);
		glBindBuffer(GL_ARRAY_BUFFER, shape_vbo);

		GLuint attrib_index;
		attrib_index = glGetAttribLocation(shader, "vertex");
		glEnableVertexArrayAttrib(shape_vao, attrib_index);
		glVertexAttribPointer(attrib_index, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

		attrib_index = glGetAttribLocation(shader, "offset");
		glEnableVertexArrayAttrib(shape_vao, attrib_index);
		glVertexAttribPointer(attrib_index, 1, GL_FLOAT, GL_FALSE, 0, (const void*)sizeof(vertices));

		// Shape 2
		const float offsets2[] {
			-0.25, -0.25, -0.25,
			-0.25, -0.25, -0.25
		};

		glGenBuffers(1, &shape2_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, shape2_vbo);
		glNamedBufferStorage(shape2_vbo, sizeof(vertices) + sizeof(offsets2), nullptr, GL_DYNAMIC_STORAGE_BIT);
		glNamedBufferSubData(shape2_vbo, 0, sizeof(vertices), vertices);
		glNamedBufferSubData(shape2_vbo, sizeof(vertices), sizeof(offsets2), offsets2);

		glGenVertexArrays(1, &shape2_vao);
		glBindVertexArray(shape2_vao);
		glBindBuffer(GL_ARRAY_BUFFER, shape2_vbo);

		attrib_index = glGetAttribLocation(shader2, "vertex");
		glEnableVertexArrayAttrib(shape2_vao, attrib_index);
		glVertexAttribPointer(attrib_index, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

		attrib_index = glGetAttribLocation(shader2, "offset");
		glEnableVertexArrayAttrib(shape2_vao, attrib_index);
		glVertexAttribPointer(attrib_index, 1, GL_FLOAT, GL_FALSE, 0, (const void*)sizeof(vertices));
	}

	void run()
	{
		glClearColor(0.6, 0.6, 0.8, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(shader);
		glBindVertexArray(shape_vao);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glUseProgram(shader2);
		glBindVertexArray(shape2_vao);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glFinish();
	}

public:
	void write()
	{
		auto rowsp = new png_bytep[height];
		for (int j : std::views::iota(0, height))
			rowsp[height-j-1] = reinterpret_cast<png_bytep>(&buffer32[j * width]);

		png_set_rows(pngp, png_infop, rowsp);
		png_write_png(pngp, png_infop, PNG_TRANSFORM_IDENTITY, nullptr);

		delete[] rowsp;
	}

	Container(int width, int height, std::string filename)
		:width(width), height(height), filename(filename)
	{
	}

	~Container()
	{
		// libpng
		if (png_infop)
			png_destroy_info_struct(pngp, &png_infop);
		if (pngp)
			png_destroy_write_struct(&pngp, nullptr);
		if (fp)
			fclose(fp);

		// OpenGL
		if (buffer)
			delete[] buffer;
		if (ctx)
		{
			OSMesaMakeCurrent(ctx, 0, 0, 0, 0);
			OSMesaDestroyContext(ctx);
		}
	}

	void init()
	{
		init_gl();
		init_libpng();
		init_user();
	}

	void init_libpng()
	{
		fp = fopen(filename.c_str(), "wb");
		if (!fp)
			throw std::format("Couldn't open output file {}", filename);

		pngp = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
		if (!pngp)
			throw "Unable to create the pngp"s;

		if (setjmp(png_jmpbuf(pngp)))
			throw "setjmp for libpng failed"s;

		png_infop = png_create_info_struct(pngp);
		if (!png_infop)
			throw "Unable to create png_infop"s;

		png_init_io(pngp, fp);

		png_set_IHDR(pngp, png_infop, width, height, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	}

	void init_gl()
	{
		// Basics
		const int attrib_list[] {
			OSMESA_FORMAT, OSMESA_RGBA,
			OSMESA_PROFILE, OSMESA_CORE_PROFILE,
			OSMESA_DEPTH_BITS, 24,
			OSMESA_STENCIL_BITS, 0,
			OSMESA_ACCUM_BITS, 0,
			OSMESA_CONTEXT_MAJOR_VERSION, 4,
			OSMESA_CONTEXT_MINOR_VERSION, 5,
			0
		};
		ctx = OSMesaCreateContextAttribs(attrib_list, nullptr);
		if (!ctx)
			throw "Context creation failed"s;

		buffer_stride = 4 * width;
		buffer_size = buffer_stride * height;
		buffer = new GLubyte[buffer_size];

		if (!OSMesaMakeCurrent(ctx, buffer, GL_UNSIGNED_BYTE, width, height))
			throw "Failed to make the context current"s;

		if (gladLoadGLLoader((GLADloadproc)OSMesaGetProcAddress) == 0)
			throw "Failed to load OpenGL function pointers"s;

		// Stats
		const std::pair<GLenum, const char*> query_list[] {
			{GL_VENDOR, "Vendor"},
			{GL_RENDERER, "Renderer"},
			{GL_VERSION, "Version"},
			{GL_SHADING_LANGUAGE_VERSION, "Shading language version"},
		};
		for (auto [query, desc] : query_list)
		{
			std::println(stderr, "{}: {}", desc, (const char*)glGetString(query));
		}

		GLint nexts;
		glGetIntegerv(GL_NUM_EXTENSIONS, &nexts);
		if (nexts > 0)
		{
			std::println(stderr, "Discovered {} extensions", nexts);
			if (false)
			for (int i : std::views::iota(0, nexts))
			{
				std::println(stderr, "{}", (const char*)glGetStringi(GL_EXTENSIONS, i));
			}
		}

		// Debug output
		glEnable(GL_DEBUG_OUTPUT);
		glDebugMessageCallback(debug_callback, this);

		// View
		glViewport(0, 0, width, height);

		// Depth testing
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
	}

private:
	static void debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* data)
	{
		std::println(stderr, "GL debug callback: source: {:#x}, type: {:#x}, severity: {:#x}: {}", source, type, severity, message);
	}

private:
	// OpenGL
	OSMesaContext ctx = nullptr;

	int width, height;
	union {
		GLubyte* buffer;
		uint32_t* buffer32 = nullptr;
	};
	size_t buffer_stride, buffer_size;

	// libpng
	std::string filename;
	FILE* fp = nullptr;
	png_structp pngp = nullptr;
	png_infop png_infop = nullptr;
};

int main(int argc, char** argv)
{
	try
	{
		std::string out_filename = argc > 1 ? argv[1] : "/dev/shm/render.png";
		Container instance(512, 512, std::move(out_filename));
		instance.init();
		instance.run();
		instance.write();
	}
	catch (std::string& e)
	{
		std::println(stderr, "Exception: {}", e);
		return 1;
	}
	catch (std::exception& e)
	{
		std::println(stderr, "std::exception: {}", e.what());
		return 1;
	}
}
