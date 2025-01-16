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

#include "SPPugCache.h"
#include "SPPugContext.h"
#include "SPPugTemplate.h"
#include "SPFilesystem.h"

#include "SPPlatformUnistd.h"

#if LINUX
#include <sys/inotify.h>
#endif

#ifndef SP_TERMINATED_DATA
#define SP_TERMINATED_DATA(view) (view.terminated()?view.data():view.str<memory::PoolInterface>().data())
#endif

namespace STAPPLER_VERSIONIZED stappler::pug {

#if LINUX
static int s_FileNotifyMask = IN_CLOSE_WRITE;
#endif

Rc<FileRef> CacheFile::read(memory::pool_t *p, FilePath path, Template::Options opts, const Callback<void(const StringView &)> &cb, int watch, int wId) {
	auto fpath = path.get();
	if (filesystem::exists(fpath)) {
		return Rc<FileRef>::alloc(p, path, opts, cb, watch, wId);
	}

	return nullptr;
}

Rc<FileRef> CacheFile::read(memory::pool_t *p, String && content, bool isTemplate, Template::Options opts, const Callback<void(const StringView &)> &cb) {
	return Rc<FileRef>::alloc(p, move(content), isTemplate, opts, cb);
}

CacheFile::CacheFile(memory::pool_t *pool, const FilePath &path, Template::Options opts, const Callback<void(const StringView &)> &cb, int watch, int wId)
: _pool(pool), _opts(opts) {
	auto fpath = path.get();

	filesystem::Stat stat;
	filesystem::stat(fpath, stat);

	_mtime = stat.mtime;
	_content.resize(stat.size);
	filesystem::readIntoBuffer((uint8_t *)_content.data(), fpath);

	if (_content.size() > 0) {
		if (wId < 0 && watch >= 0) {
#if LINUX
			_watch = inotify_add_watch(watch, SP_TERMINATED_DATA(fpath), s_FileNotifyMask);
			if (_watch == -1 && errno == ENOSPC) {
				cb("inotify limit is reached: fall back to timed watcher");
			}
#endif
		} else {
			_watch = wId;
		}
		_valid = true;
	}
	if (_valid && (fpath.ends_with(".pug") || fpath.ends_with(".stl") || fpath.ends_with(".spug"))) {
		_template = Template::read(_pool, _content, opts, cb);
		if (!_template) {
			_valid = false;
		}
	}
}

CacheFile::CacheFile(memory::pool_t *pool, String &&src, bool isTemplate, Template::Options opts, const Callback<void(const StringView &)> &cb)
: _pool(pool), _content(move(src)), _opts(opts) {
	if (_content.size() > 0) {
		_valid = true;
	}
	if (isTemplate && _valid) {
		_template = Template::read(_pool, _content, opts, cb);
		if (!_template) {
			_valid = false;
		}
	}
}

CacheFile::~CacheFile() { }

StringView CacheFile::getContent() const {
	return _content;
}

const Template *CacheFile::getTemplate() const {
	return _template;
}

int CacheFile::getWatch() const {
	return _watch;
}

Time CacheFile::getMtime() const {
	return _mtime;
}

bool CacheFile::isValid() const {
	return _valid;
}

const Template::Options &CacheFile::getOpts() const {
	return _opts;
}

int CacheFile::regenerate(int notify, StringView fpath) {
	if (_watch >= 0) {
#if LINUX
		inotify_rm_watch(notify, _watch);
		_watch = inotify_add_watch(notify, SP_TERMINATED_DATA(fpath), s_FileNotifyMask);
		return _watch;
#endif
	}
	return 0;
}

Cache::Cache(Template::Options opts, const Function<void(const StringView &)> &err)
: _pool(memory::pool::acquire()), _opts(opts), _errorCallback(err) {
#if LINUX
	_inotify = inotify_init1(IN_NONBLOCK);
#endif
	if (_inotify != -1) {
		_inotifyAvailable = true;
	}
}

Cache::~Cache() {
	if (_inotify > 0) {
#if LINUX
		for (auto &it : _templates) {
			auto fd = it.second->getWatch();
			if (fd >= 0) {
				inotify_rm_watch(_inotify, fd);
			}
		}
		close(_inotify);
#endif
	}
}

void Cache::update(int watch, bool regenerate) {
	std::unique_lock<Mutex> lock(_mutex);
	auto it = _watches.find(watch);
	if (it != _watches.end()) {
		auto tIt = _templates.find(it->second);
		if (tIt != _templates.end()) {
			if (regenerate) {
				_watches.erase(it);
				if (auto tpl = openTemplate(it->second, -1, tIt->second->getOpts())) {
					tIt->second = tpl;
					watch = tIt->second->getWatch();
					if (watch < 0) {
						_inotifyAvailable = false;
					} else {
						_watches.emplace(watch, tIt->first);
					}
				}
			} else {
				if (auto tpl = openTemplate(it->second, tIt->second->getWatch(), tIt->second->getOpts())) {
					tIt->second = tpl;
				}
			}
		}
	}
}

void Cache::update(memory::pool_t *pool, bool force) {
	memory::pool::context ctx(pool);
	for (auto &it : _templates) {
		if (it.second->getMtime() != Time()) {
			filesystem::Stat stat;
			filesystem::stat(it.first, stat);
			if (stat.mtime != it.second->getMtime() || force) {
				if (auto tpl = openTemplate(it.first, -1, it.second->getOpts())) {
					it.second = tpl;
				}
			}
		}
	}
}

int Cache::getNotify() const {
	return _inotify;
}

bool Cache::isNotifyAvailable() {
	std::unique_lock<Mutex> lock(_mutex);
	return _inotifyAvailable;
}

void Cache::regenerate(StringView key) {
	if (_inotifyAvailable) {
		auto it = _templates.find(key);
		if (it != _templates.end()) {
			_watches.erase(it->second->getWatch());
			auto watch = it->second->regenerate(_inotify, key);
			_watches.emplace(watch, it->first);
		}
	}
}

void Cache::drop(StringView key) {
	auto it = _templates.find(key);
	if (it != _templates.end()) {
		_templates.erase(it);
	}
}

bool Cache::runTemplate(const StringView &ipath, const RunCallback &cb, const OutStream &out) {
	Rc<FileRef> tpl = acquireTemplate(ipath, true, _opts);
	if (!tpl) {
		tpl = acquireTemplate(filesystem::writablePath<memory::PoolInterface>(ipath), false, _opts);
	}

	return runTemplate(tpl, ipath, cb, out, tpl->getTemplate()->getOptions());
}

bool Cache::runTemplate(const StringView &ipath, const RunCallback &cb, const OutStream &out, Template::Options opts) {
	Rc<FileRef> tpl = acquireTemplate(ipath, true, opts);
	if (!tpl) {
		tpl = acquireTemplate(filesystem::writablePath<memory::PoolInterface>(ipath), false, opts);
	}

	return runTemplate(tpl, ipath, cb, out, opts);
}

bool Cache::addFile(StringView path) {
	std::unique_lock<Mutex> lock(_mutex);
	auto it = _templates.find(path);
	if (it == _templates.end()) {
		memory::pool::context ctx(_pool);
		if (auto tpl = openTemplate(path, -1, _opts)) {
			auto it = _templates.emplace(path.pdup(_templates.get_allocator()), tpl).first;
			if (tpl->getWatch() >= 0) {
				_watches.emplace(tpl->getWatch(), it->first);
			}
			return true;
		}
	} else {
		onError(string::toString<memory::PoolInterface>("Already added: '", path, "'"));
	}
	return false;
}

bool Cache::addContent(StringView key, String &&data) {
	std::unique_lock<Mutex> lock(_mutex);
	auto it = _templates.find(key);
	if (it == _templates.end()) {
		auto tpl = CacheFile::read(_pool, move(data), false, _opts);
		_templates.emplace(key.pdup(_templates.get_allocator()), tpl);
		return true;
	} else {
		onError(string::toString<memory::PoolInterface>("Already added: '", key, "'"));
	}
	return false;
}

bool Cache::addTemplate(StringView key, String &&data) {
	return addTemplate(key, move(data), _opts);
}

bool Cache::addTemplate(StringView key, String &&data, Template::Options opts) {
	std::unique_lock<Mutex> lock(_mutex);
	auto it = _templates.find(key);
	if (it == _templates.end()) {
		auto tpl = CacheFile::read(_pool, move(data), true, opts, [&] (const StringView &err) SP_COVERAGE_TRIVIAL {
			std::cout << key << ":\n";
			std::cout << err << "\n";
		});
		_templates.emplace(key.pdup(_templates.get_allocator()), tpl);
		return true;
	} else {
		onError(string::toString<memory::PoolInterface>("Already added: '", key, "'"));
	}
	return false;
}

Rc<FileRef> Cache::get(StringView key) const {
	auto it = _templates.find(key);
	if (it != _templates.end()) {
		return it->second;
	}
	return nullptr;
}

Rc<FileRef> Cache::acquireTemplate(StringView path, bool readOnly, const Template::Options &opts) {
	std::unique_lock<Mutex> lock(_mutex);
	auto it = _templates.find(path);
	if (it != _templates.end()) {
		return it->second;
	} else if (!readOnly) {
		if (auto tpl = openTemplate(path, -1, opts)) {
			auto it = _templates.emplace(path.pdup(_templates.get_allocator()), tpl).first;
			if (tpl->getWatch() >= 0) {
				_watches.emplace(tpl->getWatch(), it->first);
			}
			return tpl;
		}
	}
	return nullptr;
}

Rc<FileRef> Cache::openTemplate(StringView path, int wId, const Template::Options &opts) {
	auto ret = CacheFile::read(_pool, FilePath(path), opts, [&] (const StringView &err) SP_COVERAGE_TRIVIAL {
		std::cout << path << ":\n";
		std::cout << err << "\n";
	}, _inotify, wId);
	if (!ret) {
		onError(string::toString<memory::PoolInterface>("File not found: ", path));
	}  else if (ret->isValid()) {
		return ret;
	}
	return nullptr;
}

bool Cache::runTemplate(Rc<FileRef> tpl, StringView ipath, const RunCallback &cb, const OutStream &out, Template::Options opts) {
	if (tpl) {
		if (auto t = tpl->getTemplate()) {
			auto iopts = tpl->getOpts();
			Context exec;
			exec.loadDefaults();
			exec.setIncludeCallback([this, iopts] (const StringView &path, Context &exec, const OutStream &out, Template::RunContext &rctx) -> bool {
				Rc<FileRef> tpl = acquireTemplate(path, true, iopts);
				if (!tpl) {
					tpl = acquireTemplate(filesystem::writablePath<memory::PoolInterface>(path), false, iopts);
				}

				if (!tpl) {
					return false;
				}

				bool ret = false;
				if (const Template *t = tpl->getTemplate()) {
					ret = t->run(exec, out, rctx);
				} else {
					out << tpl->getContent();
					ret = true;
				}

				return ret;
			});
			if (cb != nullptr) {
				if (!cb(exec, *t)) {
					return false;
				}
			}
			return t->run(exec, out, opts);
		} else {
			onError(string::toString<memory::PoolInterface>("File '", ipath, "' is not executable"));
		}
	} else {
		onError(string::toString<memory::PoolInterface>("No template '", ipath, "' found"));
	}
	return false;
}

void Cache::onError(const StringView &str) {
	if (str == "inotify limit is reached: fall back to timed watcher") {
		std::unique_lock<Mutex> lock(_mutex);
		_inotifyAvailable = false;
	}
	if (_errorCallback != nullptr) {
		_errorCallback(str);
	} else {
		std::cout << str;
	}
}

}
