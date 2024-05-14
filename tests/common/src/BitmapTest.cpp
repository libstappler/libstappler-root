/**
 Copyright (c) 2024 Stappler LLC <admin@stappler.dev>

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 **/

#include "SPCommon.h"
#include "SPBitmap.h"
#include "Test.h"

#include "png.h"

namespace STAPPLER_VERSIONIZED stappler::app::test {

namespace custom {

using namespace bitmap;

static const unsigned char CUSTOM_SIGNATURE[] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x00};
static const unsigned char PNG_SIGNATURE[] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};

struct ReadState {
	const uint8_t *data = nullptr;
	size_t offset = 0;
};

static bool isPng(const uint8_t * data, size_t dataLen) {
	if (dataLen <= 8) {
		return false;
	}

	return memcmp(CUSTOM_SIGNATURE, data, sizeof(CUSTOM_SIGNATURE)) == 0;
}

static bool getPngImageSize(const io::Producer &file, StackBuffer<512> &data, uint32_t &width, uint32_t &height) {
	if (isPng(data.data(), data.size()) && data.size() >= 24) {
		auto reader = BytesViewNetwork(data.data() + 16, 8);

		width = reader.readUnsigned32();
		height = reader.readUnsigned32();

		return true;
	}
	return false;
}

static void readDynamicData(png_structp pngPtr, png_bytep data, png_size_t length) {
	auto state = (ReadState *)png_get_io_ptr(pngPtr);
	memcpy(data, state->data + state->offset, length);
	state->offset += length;
}

struct PngReadStruct {
	~PngReadStruct() {
		if (png_ptr || info_ptr) {
			png_destroy_read_struct(png_ptr ? &png_ptr : nullptr, info_ptr ? &info_ptr : nullptr, NULL);
			png_ptr = nullptr;
			info_ptr = nullptr;
		}
	}

	PngReadStruct() { }

	bool init(const uint8_t *inputData, size_t size) {
		memcpy((uint8_t *)inputData, PNG_SIGNATURE, sizeof(PNG_SIGNATURE));

		png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		if (png_ptr == NULL) {
			log::error("libpng", "fail to create read struct");
			return false;
		}

		info_ptr = png_create_info_struct(png_ptr);
		if (info_ptr == NULL) {
			log::error("libpng", "fail to create info struct");
			png_destroy_read_struct(&png_ptr, NULL, NULL);
			return false;
		}

		if (setjmp(png_jmpbuf(png_ptr))) {
			log::error("libpng", "error in processing (setjmp return)");
			return false;
		}

		state.data = inputData;
		state.offset = 0;
		png_set_read_fn(png_ptr,(png_voidp)&state, readDynamicData);

#ifdef PNG_ARM_NEON_API_SUPPORTED
		png_set_option(png_ptr, PNG_ARM_NEON, PNG_OPTION_ON);
#endif
		png_read_info(png_ptr, info_ptr);
		return true;
	}

	bool info(ImageInfo &info) {
		if (setjmp(png_jmpbuf(png_ptr))) {
			log::error("libpng", "error in processing (setjmp return)");
			return false;
		}

		info.width = png_get_image_width(png_ptr, info_ptr);
		info.height = png_get_image_height(png_ptr, info_ptr);

		png_byte bitdepth = png_get_bit_depth(png_ptr, info_ptr);
		png_uint_32 color_type = png_get_color_type(png_ptr, info_ptr);

		if (color_type == PNG_COLOR_TYPE_PALETTE) {
			png_set_palette_to_rgb(png_ptr);
		}
		if (color_type == PNG_COLOR_TYPE_GRAY && bitdepth < 8) {
			bitdepth = 8;
			png_set_expand_gray_1_2_4_to_8(png_ptr);
		}
		if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
			png_set_tRNS_to_alpha(png_ptr);
		}
		if (bitdepth == 16) {
			png_set_strip_16(png_ptr);
		}
		if (bitdepth < 8) {
			png_set_packing(png_ptr);
		}

		png_read_update_info(png_ptr, info_ptr);
		bitdepth = png_get_bit_depth(png_ptr, info_ptr);
		color_type = png_get_color_type(png_ptr, info_ptr);
		auto rowbytes = png_get_rowbytes(png_ptr, info_ptr);

		if (color_type == PNG_COLOR_TYPE_GRAY) {
			info.color = (info.color == PixelFormat::A8?PixelFormat::A8:PixelFormat::I8);
		} else if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
			info.color = PixelFormat::IA88;
		} else if (color_type == PNG_COLOR_TYPE_RGB) {
			info.color = PixelFormat::RGB888;
		} else if (color_type == PNG_COLOR_TYPE_RGBA) {
			info.color = PixelFormat::RGBA8888;
		} else {
			info.width = 0;
			info.height = 0;
			info.stride = 0;
			log::format(log::Error, "libpng", "unsupported color type: %u", (unsigned int)color_type);
			return false;
		}

		info.stride = (uint32_t)rowbytes;

		if (info.color == PixelFormat::I8 || info.color == PixelFormat::RGB888) {
			info.alpha = AlphaFormat::Opaque;
		} else {
			info.alpha = AlphaFormat::Unpremultiplied;
		}

		return true;
	}

	bool load(BitmapWriter &outputData) {
		outputData.assign(outputData.target, PNG_SIGNATURE, 0);

		if (!info(outputData)) {
			return false;
		}

		if (setjmp(png_jmpbuf(png_ptr))) {
			log::error("libpng", "error in processing (setjmp return)");
			return false;
		}

		if (outputData.getStride) {
			auto rowbytes = png_get_rowbytes(png_ptr, info_ptr);
			outputData.stride = max((uint32_t)outputData.getStride(outputData.target, outputData.color, outputData.width), (uint32_t)rowbytes);
		}

		png_bytep row_pointers[outputData.height];

		auto dataLen = outputData.stride * outputData.height;

		outputData.resize(outputData.target, dataLen);

		for (uint32_t i = 0; i < outputData.height; ++i) {
			row_pointers[i] = outputData.getData(outputData.target, i * outputData.stride);
		}

		png_read_image(png_ptr, row_pointers);
		png_read_end(png_ptr, nullptr);
		return true;
	}

	ReadState state;
	png_structp png_ptr = nullptr;
	png_infop info_ptr = nullptr;
};

struct PngWriteStruct {
	int bit_depth = 8;
	png_structp png_ptr = nullptr;
	png_infop info_ptr = nullptr;
	bool valid = false;

	FILE *fp = nullptr;
	BitmapWriter *out = nullptr;

	~PngWriteStruct() {
		if (png_ptr != nullptr) {
			png_destroy_write_struct(&png_ptr, &info_ptr);
		}

		if (fp) {
			fclose(fp);
		}
	}

	PngWriteStruct() {
		png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		if (png_ptr == nullptr) {
			log::error("libpng", "fail to create write struct");
			return;
		}

		info_ptr = png_create_info_struct (png_ptr);
		if (info_ptr == nullptr) {
			log::error("libpng", "fail to create info struct");
			return;
		}
	}

	PngWriteStruct(BitmapWriter *v) : PngWriteStruct() {
		out = v;
		valid = true;
	}

	PngWriteStruct(StringView filename) : PngWriteStruct() {
		fp = filesystem::native::fopen_fn(filename, "wb");
		if (!fp) {
			log::error("libpng", "fail to open file '%s' to write png data", filename.data());
			valid = false;
			return;
		}

		valid = true;
	}

	static void writePngFn(png_structp png_ptr, png_bytep data, png_size_t length) {
		auto out = (BitmapWriter *)png_get_io_ptr(png_ptr);
		out->push(out->target, data, length);
	}

	bool write(const uint8_t *data, BitmapWriter &state, bool invert = false) {
		if (!valid) {
			return false;
		}

		if (!fp && !out) {
			return false;
		}

		if (setjmp (png_jmpbuf (png_ptr))) {
			log::error("libpng", "error in processing (setjmp return)");
			return false;
		}

		if (state.stride == 0) {
			state.stride = getBytesPerPixel(state.color) * state.width;
		}

		int color_type = 0;
		switch (state.color) {
		case PixelFormat::A8:
		case PixelFormat::I8:
			color_type = PNG_COLOR_TYPE_GRAY;
			break;
		case PixelFormat::IA88:
			color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
			break;
		case PixelFormat::RGB888:
			color_type = PNG_COLOR_TYPE_RGB;
			break;
		case PixelFormat::RGBA8888:
			color_type = PNG_COLOR_TYPE_RGBA;
			break;
		default:
			return false;
		}

		/* Set image attributes. */
		png_set_IHDR (png_ptr, info_ptr, state.width, state.height, bit_depth,
				color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

		/* Initialize rows of PNG. */
		png_byte *row_pointers[state.height];
		for (size_t i = 0; i < state.height; ++i) {
			row_pointers[i] = (png_byte *)data + (invert ? state.stride * (state.height - i - 1) : state.stride * i);
		}

		if (fp) {
			png_init_io (png_ptr, fp);
		} else {
			png_set_write_fn(png_ptr, out, &writePngFn, nullptr);
		}

		png_set_rows (png_ptr, info_ptr, row_pointers);
		png_write_png (png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
		state.assign(state.target, CUSTOM_SIGNATURE, sizeof(CUSTOM_SIGNATURE));
		return true;
	}
};

static bool infoPng(const uint8_t *inputData, size_t size, ImageInfo &outputData) {
	PngReadStruct pngStruct;
	return pngStruct.init(inputData, size) && pngStruct.info(outputData);
}

static bool loadPng(const uint8_t *inputData, size_t size, BitmapWriter &outputData) {
	PngReadStruct pngStruct;
	return pngStruct.init(inputData, size) && pngStruct.load(outputData);
}

static bool savePng(StringView filename, const uint8_t *data, BitmapWriter &state, bool invert) {
	struct WriteData {
		uint32_t offset = 0;
		Bytes data;
	};

	WriteData writeData;

	bitmap::BitmapWriter writer(state);
	writer.target = &writeData;

	writer.getStride = [] (void *, bitmap::PixelFormat fmt, uint32_t width) -> uint32_t {
		return uint32_t(width * bitmap::getBytesPerPixel(fmt));
	};
	writer.push = [] (void *ptr, const uint8_t *data, uint32_t size) {
		auto writeData = ((WriteData *)ptr);
		if (writeData->data.size() < writeData->offset + size) {
			writeData->data.resize(writeData->offset + size);
		}
		memcpy(writeData->data.data() + writeData->offset, data, size);
		writeData->offset += size;
	};
	writer.resize = [] (void *ptr, uint32_t size) {
		auto writeData = ((WriteData *)ptr);
		writeData->data.resize(size);
	};
	writer.getData = [] (void *ptr, uint32_t location) {
		auto writeData = ((WriteData *)ptr);
		return writeData->data.data() + location;
	};
	writer.assign = [] (void *ptr, const uint8_t *data, uint32_t size) {
		auto writeData = ((WriteData *)ptr);
		memcpy(writeData->data.data(), data, size);
		writeData->offset = size;
	};
	writer.clear = [] (void *ptr) { };

	PngWriteStruct s(&writer);
	s.write(data, writer, invert);
	filesystem::write(filename, writeData.data);
	return true;
}

static bool writePng(const uint8_t *data, BitmapWriter &state, bool invert) {
	PngWriteStruct s(&state);
	return s.write(data, state, invert);
}

}

static bitmap::BitmapFormat s_custom = bitmap::BitmapFormat("PNG-Custom", "image/png", &custom::isPng, &custom::getPngImageSize
		, &custom::infoPng, &custom::loadPng, &custom::writePng, &custom::savePng
);

struct BitmapTest : Test {
	BitmapTest() : Test("BitmapTest") { }

	virtual void testImage(StringView path) {
		auto data = filesystem::readIntoMemory<Interface>(path);

		uint32_t w, h;
		bitmap::isImage(path);
		bitmap::isImage(path, false);
		bitmap::isImage(data.data(), data.size());
		bitmap::isImage(data.data(), data.size(), false);
		bitmap::getImageSize(StringView(path), w, h);

		bitmap::ImageInfo info;
		bitmap::getImageInfo(data, info);

		bitmap::detectFormat(path);
		bitmap::detectFormat(data.data(), data.size());

		bitmap::check(s_custom.getName(), data.data(), data.size());

		auto f = filesystem::openForReading(path);
		if (f) {
			bitmap::isImage(f);
			bitmap::isImage(f, false);

			bitmap::getImageSize(f, w, h);

			CoderSource source(data);
			bitmap::getImageSize(source, w, h);
			bitmap::detectFormat(f);
		}

		struct WriteData {
			uint32_t offset = 0;
			Bytes data;
		};

		WriteData writeData;

		bitmap::BitmapWriter writer;
		writer.target = &writeData;

		writer.getStride = [] (void *, bitmap::PixelFormat fmt, uint32_t width) -> uint32_t {
			return uint32_t(width * bitmap::getBytesPerPixel(fmt));
		};
		writer.push = [] (void *ptr, const uint8_t *data, uint32_t size) {
			auto writeData = ((WriteData *)ptr);
			memcpy(writeData->data.data() + writeData->offset, data, size);
			writeData->offset += size;
		};
		writer.resize = [] (void *ptr, uint32_t size) {
			auto writeData = ((WriteData *)ptr);
			writeData->data.resize(size);
		};
		writer.getData = [] (void *ptr, uint32_t location) {
			auto writeData = ((WriteData *)ptr);
			return writeData->data.data() + location;
		};
		writer.assign = [] (void *ptr, const uint8_t *data, uint32_t size) {
			auto writeData = ((WriteData *)ptr);
			memcpy(writeData->data.data(), data, size);
			writeData->offset = size;
		};
		writer.clear = [] (void *ptr) { };

		if (info.format) {
			bitmap::getMimeType(info.format->getName());
			info.format->getMime();
			info.format->getCheckFn();
			info.format->getLoadFn();
			info.format->getWriteFn();
			info.format->getSaveFn();
			info.format->getSizeFn();
			info.format->getInfoFn();
			info.format->load(data.data(), data.size(), writer);
		}
	}

	virtual void runPoolTest() {
		auto png1 = filesystem::currentDir<Interface>("resources/1.png");
		auto tiff1 = filesystem::currentDir<Interface>("resources/at3_1m4_01.tif");
		auto png2custom = filesystem::currentDir<Interface>("resources/2.custom.png");

		auto imageData = filesystem::readIntoMemory<mem_pool::Interface>(png1);

		bitmap::BitmapTemplate<mem_pool::Interface> bmp(imageData);

		bmp.save(bitmap::FileFormat::Png, png1);
		bmp.save(s_custom.getName(), png2custom);
		bmp.save(bitmap::FileFormat::Tiff, toString(tiff1, ".tmp"));

		filesystem::remove(toString(tiff1, ".tmp"));

		bmp.write(bitmap::FileFormat::Png);
		bmp.write(bitmap::FileFormat::Tiff);
		bmp.write(bitmap::FileFormat::WebpLossless);
		bmp.write(bitmap::FileFormat::WebpLossy);
		bmp.write(bitmap::FileFormat::Jpeg);
		bmp.write(s_custom.getName());
	}

	virtual bool run() override {
		StringStream stream;
		size_t count = 0;
		size_t passed = 0;
		stream << "\n";

		bitmap::BitmapFormat::add(bitmap::BitmapFormat(s_custom));

		bitmap::getBytesPerPixel(bitmap::PixelFormat::Auto);

		auto png1 = filesystem::currentDir<Interface>("resources/1.png");
		auto png1gray = filesystem::currentDir<Interface>("resources/1.gray.png");
		auto png1alpha = filesystem::currentDir<Interface>("resources/1.alpha.png");
		auto png2custom = filesystem::currentDir<Interface>("resources/2.custom.png");
		auto webp1ll = filesystem::currentDir<Interface>("resources/1.lossless.webp");
		auto webp1l = filesystem::currentDir<Interface>("resources/1.lossy.webp");
		auto jpeg1 = filesystem::currentDir<Interface>("resources/1.jpeg");
		auto jpeg1gray = filesystem::currentDir<Interface>("resources/1.gray.jpeg");
		auto svg1 = filesystem::currentDir<Interface>("resources/24px.svg");
		auto svg2 = filesystem::currentDir<Interface>("resources/24px.1.svg");
		auto svg3 = filesystem::currentDir<Interface>("resources/24px.2.svg");
		auto gif1 = filesystem::currentDir<Interface>("resources/SampleGIFImage_40kbmb.gif");
		auto gif2 = filesystem::currentDir<Interface>("resources/bg-transparent.gif");
		auto gif3 = filesystem::currentDir<Interface>("resources/rotating-rainbow-colors-grayscale.gif");
		auto gif4 = filesystem::currentDir<Interface>("resources/fire-flames-transparent.gif");
		auto tiff1 = filesystem::currentDir<Interface>("resources/at3_1m4_01.tif");
		auto tiff2 = filesystem::currentDir<Interface>("resources/football_seal.tif");
		auto jpeg2 = filesystem::currentDir<Interface>("resources/Channel_digital_image_CMYK_color.jpg");

		auto imageData = filesystem::readIntoMemory<Interface>(png1);

		Bitmap bmp(imageData);

		auto bmpData = bmp.data().bytes<Interface>();

		bmp.save(webp1ll);
		bmp.save(bitmap::FileFormat::WebpLossy, webp1l);

		bmp.write(bitmap::FileFormat::Png);
		bmp.write(bitmap::FileFormat::Tiff);
		bmp.write(bitmap::FileFormat::WebpLossless);
		bmp.write(bitmap::FileFormat::WebpLossy);
		bmp.write(bitmap::FileFormat::Jpeg);
		bmp.write(s_custom.getName());

		bmp.loadBitmap(bmpData.data(), bmp.width(), bmp.height(), bmp.format(), bmp.alpha());
		bmp.loadBitmap(BytesView(bmpData), bmp.width(), bmp.height(), bmp.format(), bmp.alpha());
		bmp.loadBitmap(move(bmpData), bmp.width(), bmp.height(), bmp.format(), bmp.alpha());

		bmp.convert(bitmap::PixelFormat::RGB888);
		bmp.save(bitmap::FileFormat::Jpeg, jpeg1);
		bmp.convert(bitmap::PixelFormat::RGBA8888);

		bmp = Bitmap(imageData);
		bmp.convert(bitmap::PixelFormat::IA88);
		bmp.save(bitmap::FileFormat::Png, png1gray);
		bmp.convert(bitmap::PixelFormat::RGB888);
		bmp.getOriginalFormatName();
		bmp.data();

		bmp = Bitmap(imageData);
		bmp.convert(bitmap::PixelFormat::A8);
		bmp.save(png1alpha);
		bmp.save(jpeg1gray);
		bmp.convert(bitmap::PixelFormat::I8);
		bmp.convert(bitmap::PixelFormat::RGB888);

		bmp = Bitmap(imageData);
		bmp.convert(bitmap::PixelFormat::I8);
		bmp.convert(bitmap::PixelFormat::RGBA8888);

		bmp = Bitmap(imageData);
		bmp.convert(bitmap::PixelFormat::IA88);
		bmp.convert(bitmap::PixelFormat::RGBA8888);

		bmp = Bitmap(imageData);
		bmp.convert(bitmap::PixelFormat::I8);
		bmp.convert(bitmap::PixelFormat::IA88);

		bmp = Bitmap(imageData);
		bmp.convert(bitmap::PixelFormat::A8);
		bmp.convert(bitmap::PixelFormat::IA88);

		bmp = Bitmap(imageData);
		bmp.convert(bitmap::PixelFormat::A8);
		bmp.convert(bitmap::PixelFormat::RGB888);
		bmp.convert(bitmap::PixelFormat::RGB888);

		bmp = Bitmap(imageData);
		bmp.convert(bitmap::PixelFormat::RGB888);
		bmp.convert(bitmap::PixelFormat::A8);

		bmp = Bitmap(imageData);
		bmp.convert(bitmap::PixelFormat::A8);
		bmp.convert(bitmap::PixelFormat::RGBA8888);
		bmp.convert(bitmap::PixelFormat::RGBA8888);

		bmp = Bitmap(imageData);
		bmp.convert(bitmap::PixelFormat::IA88);
		bmp.convert(bitmap::PixelFormat::A8);
		bmp.convert(bitmap::PixelFormat::A8);

		bmp = Bitmap(imageData);
		bmp.convert(bitmap::PixelFormat::IA88);
		bmp.convert(bitmap::PixelFormat::I8);

		bmp = Bitmap(imageData);
		bmp.convert(bitmap::PixelFormat::RGB888);
		bmp.convert(bitmap::PixelFormat::I8);
		bmp.convert(bitmap::PixelFormat::I8);

		bmp = Bitmap(imageData);

		Bitmap bmp2(move(bmp));
		bmp2.save(s_custom.getName(), png2custom);
		bmp2.convert(bitmap::PixelFormat::RGB888);
		bmp2.convert(bitmap::PixelFormat::IA88);
		bmp2.convert(bitmap::PixelFormat::IA88);
		bmp2.updateStride([] (bitmap::PixelFormat fmt, uint32_t w) -> uint32_t {
			return w * 4;
		});

		imageData = filesystem::readIntoMemory<Interface>(png2custom);
		bmp = Bitmap(imageData);

		testImage(png2custom);
 		testImage(svg1);
 		testImage(svg2);
 		testImage(svg3);
		testImage(png1);
		testImage(png1gray);
		testImage(png1alpha);
		testImage(webp1ll);
		testImage(webp1l);
		testImage(jpeg1);
		testImage(jpeg1gray);
		testImage(jpeg2);
		testImage(gif1);
		testImage(gif1);
		testImage(gif2);
		testImage(gif3);
		testImage(gif4);
		testImage(tiff1);
		testImage(tiff2);

		auto tiffs = filesystem::currentDir<Interface>("resources/tiff");
		filesystem::ftw(tiffs, [] (StringView path, bool isFile) {
			std::cout << path << "\n";
			uint32_t w, h;
			bitmap::getImageSize(path, w, h);
		});

		auto p = memory::pool::create(memory::app_root_pool);
		memory::pool::push(p);

		runPoolTest();

		memory::pool::pop();

		_desc = stream.str();

		return count == passed;
	}
} _BitmapTest;

}
