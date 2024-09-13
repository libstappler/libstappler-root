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

#ifndef EXTRA_WEBSERVER_WEBSERVER_SERVER_SPWEBHOSTCOMPONENT_H_
#define EXTRA_WEBSERVER_WEBSERVER_SERVER_SPWEBHOSTCOMPONENT_H_

#include "SPWebHost.h"

namespace STAPPLER_VERSIONIZED stappler::web {

class SP_PUBLIC HostComponent : public AllocBase {
public:
	using Symbol = HostComponent * (*) (const Host &serv, const HostComponentInfo &dict);
	using Scheme = db::Scheme;

	using CommandCallback = Function<bool(StringView, const Callback<void(const Value &)> &)>;

	struct Loader {
		StringView name;
		Symbol loader;

		Loader(const StringView &, Symbol);
	};

	struct Command {
		String name;
		String desc;
		String help;
		CommandCallback callback;
	};

	HostComponent(const Host &serv, const HostComponentInfo &);
	virtual ~HostComponent() { }

	virtual void handleChildInit(const Host &);
	virtual void handleStorageInit(const Host &, const db::Adapter &);
	virtual void initTransaction(db::Transaction &);
	virtual void handleHeartbeat(const Host &);

	const Value & getConfig() const { return _config; }
	StringView getName() const { return _name; }
	StringView getVersion() const { return _version; }

	const db::Scheme * exportScheme(const db::Scheme &);

	void addCommand(const StringView &name, Function<Value(const StringView &)> &&,
			const StringView &desc = StringView(), const StringView &help = StringView());
	void addOutputCommand(const StringView &name, CommandCallback &&,
			const StringView &desc = StringView(), const StringView &help = StringView());
	const Command *getCommand(const StringView &name) const;

	const Map<String, Command> &getCommands() const;

	template <typename Value>
	void exportValues(Value &&val) {
		exportScheme(val);
	}

	template <typename Value, typename ... Args>
	void exportValues(Value &&val, Args && ... args) {
		exportValues(std::forward<Value>(val));
		exportValues(std::forward<Args>(args)...);
	}

	Host getHost() const { return _host; }

protected:
	Map<String, Command> _commands;

	Host _host;
	String _name;
	String _version;
	Value _config;
};

}

#endif /* EXTRA_WEBSERVER_WEBSERVER_SERVER_SPWEBHOSTCOMPONENT_H_ */
