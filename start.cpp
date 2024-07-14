#include <print>
#include <string>
#include <ranges>
#include <cstdio>
#include <cstring>

#include <GL/osmesa.h>
#include <png.h>

using namespace std::literals::string_literals;

class Container
{
public:
	void write()
	{
		auto rowsp = new png_bytep[height];
		for (int j : std::views::iota(0, height))
			rowsp[j] = static_cast<png_bytep>(&buffer[j * width]);

		png_set_rows(pngp, png_infop, rowsp);
		png_write_png(pngp, png_infop, PNG_TRANSFORM_IDENTITY, nullptr);

		delete[] rowsp;
	}

	void run()
	{
		std::memset(buffer, 0xff, buffer_size);
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
			OSMesaDestroyContext(ctx);
	}

	void init()
	{
		init_gl();
		init_libpng();
	}

	void init_libpng()
	{
		fp = fopen(filename.c_str(), "wb");
		if (!fp)
			throw std::format("Couldn't open output file {}", filename);

		pngp = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
		if (!pngp)
			throw "Unable to create the pngp"s;

		png_infop = png_create_info_struct(pngp);
		if (!png_infop)
			throw "Unable to create png_infop"s;

		png_init_io(pngp, fp);

		png_set_IHDR(pngp, png_infop, width, height, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	}

	void init_gl()
	{
		// Basics
		const int attrib_list[] {
			OSMESA_FORMAT, OSMESA_RGBA,
			OSMESA_PROFILE, OSMESA_CORE_PROFILE,
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
			throw "Failed making the context current"s;

		load_gl_functions();

		// Stats
		const std::pair<GLenum, const char*> query_list[] {
			{GL_VENDOR, "Vendor"},
			{GL_RENDERER, "Renderer"},
			{GL_VERSION, "Version"},
			{GL_SHADING_LANGUAGE_VERSION, "Shading language version"},
		};
		for (auto [query, desc] : query_list)
		{
			std::println(stderr, "{}: {}", desc, (const char*)_glGetString(query));
		}

		GLint nexts;
		_glGetIntegerv(GL_NUM_EXTENSIONS, &nexts);
		if (nexts > 0)
		{
			std::println(stderr, "Discovered {} extensions", nexts);
			if (false)
			for (int i : std::views::iota(0, nexts))
			{
				std::println(stderr, "{}", (const char*)_glGetStringi(GL_EXTENSIONS, i));
			}
		}

		// Debug output
		_glEnable(GL_DEBUG_OUTPUT);
		_glDebugMessageCallback(debug_callback, this);
	}

	void load_gl_functions()
	{
		auto load = [](auto& pfn, const char* name) {
			pfn = reinterpret_cast<std::remove_reference<decltype(pfn)>::type>(OSMesaGetProcAddress(name));
			if (!pfn)
				throw std::format("Couldn't obtain the address of function {}", name);
			std::println(stderr, "Loaded function: {}: {}", name, (void*)pfn);
		};

#define _LOAD(name) load(_##name, #name)

		_LOAD(glGetString);
		_LOAD(glGetStringi);
		_LOAD(glGetIntegerv);
		_LOAD(glEnable);
		_LOAD(glDisable);
		_LOAD(glDebugMessageInsert);
		_LOAD(glDebugMessageCallback);

#undef _LOAD
	}

private:
	static void debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* data)
	{
		std::println(stderr, "Debug callback: source: {:#x}, type: {:#x}, severity: {:#x}: {}", source, type, severity, message);
	}

private:
	// OpenGL
	OSMesaContext ctx = nullptr;

	int width, height;
	GLubyte* buffer = nullptr;
	size_t buffer_stride, buffer_size;

	// libpng
	std::string filename;
	FILE* fp = nullptr;
	png_structp pngp = nullptr;
	png_infop png_infop = nullptr;

private:
	GLubyte* (*_glGetString) (GLenum name);
	GLubyte* (*_glGetStringi) (GLenum name, GLuint index);
	void (*_glGetIntegerv) (GLenum pname, GLint* data);
	void (*_glEnable) (GLenum cap);
	void (*_glDisable) (GLenum cap);
	void (*_glDebugMessageInsert) (GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* message);
	void (*_glDebugMessageCallback) (void (*debugproc) (GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* data), void* data);
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
