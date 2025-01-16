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

#ifndef EXTRA_WEBSERVER_PUG_SPPUGCACHE_H_
#define EXTRA_WEBSERVER_PUG_SPPUGCACHE_H_

#include "SPPug.h"
#include "SPPugTemplate.h"

namespace STAPPLER_VERSIONIZED stappler::pug {

class CacheFile;

using FileRef = SharedRef<CacheFile>;

class SP_PUBLIC CacheFile : public memory::PoolInterface::AllocBaseType {
public:
	static Rc<FileRef> read(memory::pool_t *, FilePath path, Template::Options opts = Template::Options::getDefault(),
			const Callback<void(const StringView &)> & = nullptr, int watch = -1, int wId = -1);

	static Rc<FileRef> read(memory::pool_t *, String && content, bool isTemplate, Template::Options opts = Template::Options::getDefault(),
			const Callback<void(const StringView &)> & = nullptr);

	StringView getContent() const;
	const Template *getTemplate() const;
	int getWatch() const;
	Time getMtime() const;
	bool isValid() const;

	const Template::Options &getOpts() const;

	int regenerate(int notify, StringView);

	CacheFile(memory::pool_t *, const FilePath &path, Template::Options opts, const Callback<void(const StringView &)> &cb, int watch, int wId);
	CacheFile(memory::pool_t *, String && content, bool isTemplate, Template::Options opts, const Callback<void(const StringView &)> &cb);

	virtual ~CacheFile();

protected:
	int _watch = -1;
	memory::pool_t *_pool = nullptr;
	Time _mtime = Time();
	String _content;
	Template * _template = nullptr;
	Template::Options _opts;
	bool _valid = false;
};

class SP_PUBLIC Cache : public memory::AllocPool {
public:
	using OutStream = Callback<void(StringView)>;
	using RunCallback = Callback<bool(Context &, const Template &)>;
	using Options = Template::Options;

	Cache(Template::Options opts = Template::Options::getDefault(), const Function<void(const StringView &)> &err = nullptr);
	~Cache();

	bool runTemplate(const StringView &, const RunCallback &, const OutStream &);
	bool runTemplate(const StringView &, const RunCallback &, const OutStream &, Template::Options opts);

	bool addFile(StringView);
	bool addContent(StringView, String &&);
	bool addTemplate(StringView, String &&);
	bool addTemplate(StringView, String &&, Template::Options opts);

	Rc<FileRef> get(StringView key) const;

	void update(int watch, bool regenerate);
	void update(memory::pool_t *, bool force = false);

	int getNotify() const;
	bool isNotifyAvailable();

	void regenerate(StringView);
	void drop(StringView);

protected:
	Rc<FileRef> acquireTemplate(StringView, bool readOnly, const Template::Options &);
	Rc<FileRef> openTemplate(StringView, int wId, const Template::Options &);

	bool runTemplate(Rc<FileRef>, StringView ipath, const RunCallback &cb, const OutStream &out, Template::Options opts);
	void onError(const StringView &);

	int _inotify = -1;
	bool _inotifyAvailable = false;

	memory::pool_t *_pool = nullptr;
	Mutex _mutex;
	Map<StringView, Rc<FileRef>> _templates;
	Map<int, StringView> _watches;
	Template::Options _opts;
	Function<void(const StringView &)> _errorCallback;
};

}

#endif /* EXTRA_WEBSERVER_PUG_SPPUGCACHE_H_ */
