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

#include "SPWebUnixHost.h"
#include "SPWebHostComponent.h"
#include "SPDso.h"

namespace stappler::web {

UnixHostController::~UnixHostController() { }

UnixHostController::UnixHostController(Root *root, pool_t *p, UnixHostConfig &cfg)
: HostController(root, p) {
	_hostInfo.hostname = cfg.hastname.pdup(p);
	_hostInfo.admin = cfg.admin.pdup(p);
	_hostInfo.documentRoot = cfg.root.pdup(p);

	_componentsToLoad = move(cfg.components);

	if (cfg.db) {
		for (auto &it : cfg.db.asDict()) {
			_dbParams.emplace(StringView(it.first).pdup(p), StringView(it.second.getString()).pdup(p));
		}
	}
}

}
