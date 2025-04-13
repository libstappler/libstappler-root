/**
 Copyright (c) 2025 Stappler LLC <admin@stappler.dev>

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
#include "SPFilepath.h"
#include "SPFilesystem.h"
#include "SPPlatform.h"
#include "SPTime.h"
#include "SPCommandLineParser.h"
#include "SPBitmap.h"
#include "SPSharedModule.h"

namespace STAPPLER_VERSIONIZED stappler::app {

using namespace mem_std;

static constexpr auto HELP_STRING = R"(fstest <options> [<action>])";

// Опции для аргументов командной строки
static CommandLineParser<Value> CommandLine(
		{CommandLineOption<Value>{.patterns = {"-v", "--verbose"},
			 .description = "Produce more verbose output",
			 .callback = [](Value &target, StringView pattern, SpanView<StringView> args) -> bool {
				 target.setBool(true, "verbose");
				 return true;
			 }},
			CommandLineOption<Value>{.patterns = {"-h", "--help"},
				.description = StringView("Show help message and exit"),
				.callback = [](Value &target, StringView pattern,
									SpanView<StringView> args) -> bool {
					target.setBool(true, "help");
					return true;
				}}});


SP_EXTERN_C int main(int argc, const char *argv[]) {
	Value opts;
	Vector<StringView> args;

	if (!CommandLine.parse(opts, argc, argv,
				[&](Value &, StringView arg) { args.emplace_back(arg); })) {
		std::cerr << "Fail to parse command line arguments\n";
		return -1;
	}

	if (opts.getBool("help")) {
		std::cout << HELP_STRING << "\n";
		CommandLine.describe([&](StringView str) { std::cout << str; });
		return 0;
	}

	auto ret = perform_main([&]() -> int {
		using namespace bitmap;

		std::cout << getVersionDescription<Interface>(getAppconfigVersionIndex()) << "\n";

		decltype(static_cast<Pair<FileFormat, StringView> (*)(const FileInfo &)>(
				detectFormat)) _detectFormat;
		_detectFormat =
				SharedModule::acquireTypedSymbol<decltype(_detectFormat)>("bitmap", "detectFormat");

		/*SharedSymbol{"detectFormat(FileInfo const&)",
			(void *)static_cast<Pair<FileFormat, StringView> (*)(const FileInfo &)>(detectFormat)},
				SharedSymbol{"detectFormat(io::Producer const&)",
					(void *)static_cast<Pair<FileFormat, StringView> (*)(io::Producer const &)>(
							detectFormat)},
				SharedSymbol{"detectFormat(uint8_t const*, size_t)",
					(void *)static_cast<Pair<FileFormat, StringView> (*)(uint8_t const *, size_t)>(
							detectFormat)},
				SharedSymbol{"getMimeType(FileFormat)",
					(void *)static_cast<StringView (*)(FileFormat)>(getMimeType)},
				SharedSymbol{"getMimeType(StringView)",
					(void *)static_cast<StringView (*)(StringView)>(getMimeType)},
				SharedSymbol{"getImageSize(io::Producer const&,uint32_t&,uint32_t&)",
					(void *)static_cast<bool (*)(io::Producer const &, uint32_t &, uint32_t &)>(
							getImageSize)},
				SharedSymbol{"getImageSize(FileInfo const&,uint32_t&,uint32_t&)",
					(void *)static_cast<bool (*)(const FileInfo &, uint32_t &, uint32_t &)>(
							getImageSize)},*/

		std::cout << "Merge: " << filepath::merge<Interface>("/a/", "/b/") << "\n";
		std::cout << "Merge: " << filepath::merge<Interface>("/a", "/b/") << "\n";
		std::cout << "Merge: " << filepath::merge<Interface>("/a/", "b/") << "\n";
		std::cout << "Merge: " << filepath::merge<Interface>("/a", "b/") << "\n";
		std::cout << "Merge: " << filepath::merge<Interface>("a/", "/b/") << "\n";
		std::cout << "Merge: " << filepath::merge<Interface>("a", "/b/") << "\n";
		std::cout << "Merge: " << filepath::merge<Interface>("a/", "b/") << "\n";
		std::cout << "Merge: " << filepath::merge<Interface>("a", "b/") << "\n";

		String str;

		auto writerFn = [&](StringView s) { str.append(s.data(), s.size()); };

		auto mergeTest = [&](StringView first, StringView second, StringView third) -> StringView {
			str.clear();
			filepath::merge(writerFn, first, second, third);
			return str;
		};

		auto mergeTest2 = [&](StringView first, StringView second, StringView third) -> StringView {
			str.clear();
			StringView paths[] = {first, second, third};
			filepath::merge(writerFn, paths);
			return str;
		};

		auto mergeTest3 = [](StringView first, StringView second, StringView third) -> auto {
			StringView paths[] = {first, second, third};
			return filepath::merge<Interface>(paths);
		};

		auto mergeTest4 = [](StringView first, StringView second, StringView third) -> auto {
			StringView paths[] = {"/", first, second, third};
			return filepath::merge<Interface>(paths);
		};

		std::cout << "Merge5: " << mergeTest4("/a/", "/b/", "/c/") << "\n";
		std::cout << "Merge5: " << mergeTest4("/a", "/b/", "/c/") << "\n";
		std::cout << "Merge5: " << mergeTest4("/a/", "b/", "/c/") << "\n";
		std::cout << "Merge5: " << mergeTest4("/a", "b/", "/c/") << "\n";
		std::cout << "Merge5: " << mergeTest4("a/", "/b/", "/c/") << "\n";
		std::cout << "Merge5: " << mergeTest4("a", "/b/", "/c/") << "\n";
		std::cout << "Merge5: " << mergeTest4("a/", "b/", "/c/") << "\n";
		std::cout << "Merge5: " << mergeTest4("a", "b/", "/c/") << "\n";

		std::cout << "Merge2: " << mergeTest("/a/", "/b/", "/c/") << "\n";
		std::cout << "Merge2: " << mergeTest("/a", "/b/", "/c/") << "\n";
		std::cout << "Merge2: " << mergeTest("/a/", "b/", "/c/") << "\n";
		std::cout << "Merge2: " << mergeTest("/a", "b/", "/c/") << "\n";
		std::cout << "Merge2: " << mergeTest("a/", "/b/", "/c/") << "\n";
		std::cout << "Merge2: " << mergeTest("a", "/b/", "/c/") << "\n";
		std::cout << "Merge2: " << mergeTest("a/", "b/", "/c/") << "\n";
		std::cout << "Merge2: " << mergeTest("a", "b/", "/c/") << "\n";

		std::cout << "Merge3: " << mergeTest2("/a/", "/b/", "/c/") << "\n";
		std::cout << "Merge3: " << mergeTest2("/a", "/b/", "/c/") << "\n";
		std::cout << "Merge3: " << mergeTest2("/a/", "b/", "/c/") << "\n";
		std::cout << "Merge3: " << mergeTest2("/a", "b/", "/c/") << "\n";
		std::cout << "Merge3: " << mergeTest2("a/", "/b/", "/c/") << "\n";
		std::cout << "Merge3: " << mergeTest2("a", "/b/", "/c/") << "\n";
		std::cout << "Merge3: " << mergeTest2("a/", "b/", "/c/") << "\n";
		std::cout << "Merge3: " << mergeTest2("a", "b/", "/c/") << "\n";

		std::cout << "Merge4: " << mergeTest3("/a/", "/b/", "/c/") << "\n";
		std::cout << "Merge4: " << mergeTest3("/a", "/b/", "/c/") << "\n";
		std::cout << "Merge4: " << mergeTest3("/a/", "b/", "/c/") << "\n";
		std::cout << "Merge4: " << mergeTest3("/a", "b/", "/c/") << "\n";
		std::cout << "Merge4: " << mergeTest3("a/", "/b/", "/c/") << "\n";
		std::cout << "Merge4: " << mergeTest3("a", "/b/", "/c/") << "\n";
		std::cout << "Merge4: " << mergeTest3("a/", "b/", "/c/") << "\n";
		std::cout << "Merge4: " << mergeTest3("a", "b/", "/c/") << "\n";

		for (auto it : each<FileCategory>()) {
			filesystem::enumeratePaths(it, [&](StringView path, FileFlags) {
				std::cout << "enumeratePaths: " << it << ": " << path << "\n";
				return true;
			});
		}

		filesystem::detectResourceCategory("/usr/local/share/test",
				[](StringView path, StringView nonprefixed) {
			std::cout << "detectResourceCategory: /usr/local/share/test -> " << path << "\n";
		});

		for (auto it : each<FileCategory>()) {
			filesystem::enumeratePaths(FileInfo{"test", it}, [&](StringView path, FileFlags) {
				std::cout << "enumerateWritablePaths: " << it << ": " << path << "\n - "
						  << filepath::canonical<Interface>(path) << "\n";
				return true;
			});
			filesystem::enumerateWritablePaths(FileInfo{"test", it},
					[&](StringView path, FileFlags) {
				std::cout << "enumerateWritablePaths: " << it << ": " << path << "\n - "
						  << filepath::canonical<Interface>(path) << "\n";
				return true;
			});
			filesystem::enumerateWritablePaths(FileInfo{"/test", it},
					[&](StringView path, FileFlags) {
				std::cout << "enumerateWritablePaths: " << it << ": " << path << "\n - "
						  << filepath::canonical<Interface>(path) << "\n";
				return true;
			});
		}

		filesystem::remove(FileInfo("test1", FileCategory::AppData), true, true);

		filesystem::mkdir_recursive(FileInfo{"test1/test2/test3", FileCategory::AppData});

		filesystem::ftw(FileInfo("test1", FileCategory::AppData),
				[](const FileInfo &info, FileType type) {
			std::cout << "ftw: " << type << " " << info << "\n";
			return true;
		}, -1, true);

		filesystem::enumerateWritablePaths(FileInfo{"test1", FileCategory::AppData},
				filesystem::Access::Empty, [&](StringView path, FileFlags) {
			std::cout << "enumerateWritablePaths (should not appear): " << ": " << path << "\n";
			return true;
		});

		filesystem::enumerateWritablePaths(FileInfo{"test2", FileCategory::AppData},
				filesystem::Access::Empty, [&](StringView path, FileFlags) {
			std::cout << "enumerateWritablePaths: " << ": " << path << "\n";
			return true;
		});

		filesystem::copy(FileInfo{"scripts"}, FileInfo{"/", FileCategory::AppData});

		filesystem::ftw(FileInfo("scripts", FileCategory::AppData),
				[](const FileInfo &info, FileType type) {
			std::cout << "ftw: " << type << " " << info << "\n";
			return true;
		}, -1, true);

		return 0;
	});

	return ret;
}

} // namespace stappler::app
