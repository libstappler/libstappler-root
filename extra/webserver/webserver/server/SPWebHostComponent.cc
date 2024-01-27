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

#include "SPWebHostComponent.h"

namespace stappler::web {

HostComponent::Loader::Loader(const StringView &str, Symbol s) : name(str), loader(s) { }

HostComponent::HostComponent(const Host &host, const HostComponentInfo &cfg)
: _host(host)
, _name(cfg.name.str<Interface>())
, _version(cfg.version.empty() ? String("1.0") : cfg.version.str<Interface>())
, _config(cfg.data) {
	if (_config.isString("version")) {
		_version = _config.getString("version");
	}
}

void HostComponent::handleChildInit(const Host &serv) { }

void HostComponent::handleStorageInit(const Host &, const db::Adapter &) { }

void HostComponent::initTransaction(db::Transaction &) { }

void HostComponent::handleHeartbeat(const Host &) { }

const db::Scheme * HostComponent::exportScheme(const db::Scheme &scheme) {
	return _host.exportScheme(scheme);
}

void HostComponent::addCommand(const StringView &name, Function<Value(const StringView &)> &&cb, const StringView &desc, const StringView &help) {
	addOutputCommand(name, [cb] (StringView v, const Callback<void(const Value &)> &scb) -> bool {
		auto val = cb(v);
		if (!val.isNull()) {
			scb(val);
			return true;
		}
		return false;
	}, desc, help);
}

void HostComponent::addOutputCommand(const StringView &name, CommandCallback &&cb, const StringView &desc, const StringView &help) {
	_commands.emplace(name.str<Interface>(), Command{name.str<Interface>(), desc.str<Interface>(), help.str<Interface>(), move(cb)});
}

const HostComponent::Command *HostComponent::getCommand(const StringView &name) const {
	auto it = _commands.find(name);
	if (it != _commands.end()) {
		return &it->second;
	}
	return nullptr;
}

const Map<String, HostComponent::Command> &HostComponent::getCommands() const {
	return _commands;
}

}
