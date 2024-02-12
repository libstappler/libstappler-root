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

#include "SPWebTools.h"
#include "SPWebHost.h"
#include "SPWebRequestController.h"
#include "SPWebHostComponent.h"
#include "SPWebInputFilter.h"
#include "SPWebResource.h"
#include "SPWebOutput.h"
#include "SPSqlHandle.h"
#include "SPDbAdapter.h"
#include "SPValid.h"

namespace STAPPLER_VERSIONIZED stappler::web::tools {

struct SocketCommand;

class ShellSocketHandler : public WebsocketHandler {
public:
	enum class ShellMode {
		Plain,
		Html,
	};

	ShellSocketHandler(WebsocketManager *m, pool_t *pool, StringView url, int64_t userId);

	void setShellMode(ShellMode m) { _mode = m; }
	ShellMode getShellMode() const { return _mode; }

	db::User *getUser() const { return _user; }
	const Vector<SocketCommand *> &getCommands() const { return _cmds; }
	const Vector<Pair<StringView, const Map<String, HostComponent::Command> *>> &getExternals() const { return _external; }

	bool onCommand(StringView &r);

	virtual void handleBegin() override;

	// Data frame was recieved from network
	virtual bool handleFrame(WebsocketFrameType t, const Bytes &b) override;

	// Message was recieved from broadcast
	virtual bool handleMessage(const Value &val) override;

	void sendCmd(const StringView &v);
	void sendError(const String &str);
	void sendData(const Value & data);

protected:
	Vector<SocketCommand *> _cmds;
	Vector<Pair<StringView, const Map<String, HostComponent::Command> *>> _external;
	ShellMode _mode = ShellMode::Plain;
	db::User *_user = nullptr;
	int64_t _userId = 0;
};

struct SocketCommand : AllocBase {
	SocketCommand(const String &str) : name(str) { }
	virtual ~SocketCommand() { }
	virtual bool run(ShellSocketHandler &h, StringView &r) = 0;
	virtual StringView desc() const = 0;
	virtual StringView help() const = 0;

	String name;
};

struct ModeCmd : SocketCommand {
	ModeCmd() : SocketCommand("mode") { }

	virtual bool run(ShellSocketHandler &h, StringView &r) override {
		if (!r.empty()) {
			if (r.is("plain")) {
				h.setShellMode(ShellSocketHandler::ShellMode::Plain);
				h.send("You are now in plain text mode");
			} else if (r.is("html")) {
				h.setShellMode(ShellSocketHandler::ShellMode::Html);
				h.send("You are now in <font color=\"#3f51b5\">HTML</font> mode");
			}
		} else {
			if (h.getShellMode() == ShellSocketHandler::ShellMode::Plain) {
				h.send("Plain text mode");
			} else {
				h.send("<font color=\"#3f51b5\">HTML</font> mode");
			}
		}
		return true;
	}

	virtual StringView desc() const {
		return "plain|html - Switch socket output mode";
	}
	virtual StringView help() const {
		return "plain|html - Switch socket output mode";
	}
};

struct DebugCmd : SocketCommand {
	DebugCmd() : SocketCommand("debug") { }

	virtual bool run(ShellSocketHandler &h, StringView &r) override {
		if (!r.empty()) {
			if (r.is("on")) {
				Root::getCurrent()->setDebugEnabled(true);
				h.send("Try to enable debug mode, wait a second, until all servers receive your message");
			} else if (r.is("off")) {
				Root::getCurrent()->setDebugEnabled(false);
				h.send("Try to disable debug mode, wait a second, until all servers receive your message");
			}
		} else {
			if (Root::getCurrent()->isDebugEnabled()) {
				h.send("Debug mode: On");
			} else {
				h.send("Debug mode: Off");
			}
		}
		return true;
	}

	virtual StringView desc() const {
		return "on|off - Switch server debug mode";
	}
	virtual StringView help() const {
		return "on|off - Switch server debug mode";
	}
};

struct ListCmd : SocketCommand {
	ListCmd() : SocketCommand("meta") { }

	virtual bool run(ShellSocketHandler &h, StringView &r) override {
		if (r.empty()) {
			Value ret;
			auto &schemes = h.manager()->host().getSchemes();
			for (auto &it : schemes) {
				ret.addString(it.first);
			}
			h.sendData(ret);
		} else if (r == "all") {
			Value ret;
			auto &schemes = h.manager()->host().getSchemes();
			for (auto &it : schemes) {
				auto &val = ret.emplace(it.first);
				auto &fields = it.second->getFields();
				for (auto &fit : fields) {
					val.setValue(fit.second.getTypeDesc(), fit.first);
				}
			}
			h.sendData(ret);
		} else {
			Value ret;
			auto cmd = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
			auto scheme = h.manager()->host().getScheme(cmd);
			if (scheme) {
				auto &fields = scheme->getFields();
				for (auto &fit : fields) {
					ret.setValue(fit.second.getTypeDesc(), fit.first);
				}
				h.sendData(ret);
			}
		}
		return true;
	}

	virtual StringView desc() const {
		return "all|<name> - Get information about data scheme";
	}
	virtual StringView help() const {
		return "all|<name> - Get information about data scheme";
	}
};

struct ResourceCmd : SocketCommand {
	ResourceCmd(const String &str) : SocketCommand(str) { }

	const db::Scheme *acquireScheme(ShellSocketHandler &h, const StringView &scheme) {
		return h.manager()->host().getScheme(scheme);
	}

	Resource *acquireResource(const db::Transaction &t, ShellSocketHandler &h, const StringView &scheme, const StringView &path,
			const StringView &resolve, const Value &val = Value()) {
		Resource *ret = nullptr;
		if (!scheme.empty()) {
			auto s =  acquireScheme(h, scheme);
			if (s) {
				ret =  Resource::resolve(t, *s,
						path.empty()
						? String("/")
						: (path.is<StringView::CharGroup<CharGroupId::Numbers>>())
							? StringView(toString("/id", path))
							: path);
				if (ret) {
					ret->setUser(h.getUser());
					if (!resolve.empty()) {
						if (resolve.front() == '(') {
							ret->applyQuery(data::read<Interface>(resolve));
						} else {
							ret->setResolveOptions(Value(resolve));
						}
					}
					if (!val.empty()) {
						ret->applyQuery(val);
					}
					ret->prepare();
				} else {
					h.sendError(toString("Fail to resolve resource \"", path, "\" for scheme ", scheme));
				}
			} else {
				h.sendError(toString("No such scheme: ", scheme));
			}
		} else {
			h.sendError(toString("Scheme is not defined"));
		}
		return ret;
	}
};

struct GetCmd : ResourceCmd {
	GetCmd() : ResourceCmd("get") { }

	virtual bool run(ShellSocketHandler &h, StringView &r) override {
		auto schemeName = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();

		StringView path("/");
		if (r.is('/') || r.is<StringView::CharGroup<CharGroupId::Numbers>>()) {
			path = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
			r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		}

		auto resolve = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();

		h.performWithStorage([&, this] (const db::Transaction &t) {
			if (auto r = acquireResource(t, h, schemeName, path, resolve)) {
				auto data = r->getResultObject();
				h.sendData(data);
				delete r;
			}
		});
		return true;
	}

	virtual StringView desc() const {
		return "<scheme> <path> <resolve> - Get data from scheme";
	}
	virtual StringView help() const {
		return "<scheme> <path> <resolve> - Get data from scheme";
	}
};

struct HistoryCmd : ResourceCmd {
	HistoryCmd() : ResourceCmd("history") { }

	virtual bool run(ShellSocketHandler &h, StringView &r) override {
		bool ret = false;
		auto schemeName = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();

		int64_t time = r.readInteger().get(0);

		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();

		auto schemeView = schemeName;
		StringView field; uint64_t tag = 0;
		schemeName = schemeView.readUntilString("::");
		if (schemeView.is("::")) {
			schemeView += 2;
			auto tmpField = schemeView.readUntilString("::");
			if (schemeView.is("::")) {
				schemeView += 2;
				auto tmpTag = schemeView.readInteger().get(0);
				if (tmpTag > 0 && !tmpField.empty()) {
					field = tmpField;
					tag = tmpTag;
				}
			}
		}

		if (auto s = acquireScheme(h, schemeName)) {
			h.performWithStorage([&] (const db::Transaction &t) {
				if (auto a = dynamic_cast<db::sql::SqlHandle *>(t.getAdapter().getBackendInterface())) {
					if (field.empty()) {
						h.sendData(a->getHistory(*s, Time::microseconds(time), true));
					} else if (auto f = s->getField(field)) {
						if (f->getType() == db::Type::View) {
							h.sendData(a->getHistory(*static_cast<const db::FieldView *>(f->getSlot()), s, tag, Time::microseconds(time), true));
						}
					}
					ret = true;
				}
			});
		}

		h.sendError(toString("Scheme is not defined"));
		return ret;
	}

	virtual StringView desc() const {
		return "<scheme> <time> - Changelog for scheme or view";
	}
	virtual StringView help() const {
		return "\thistory <scheme|view> <time> - Changelog for scheme or view\n\n"
				"Scheme can be defined by it's name\n"
				"View can be defined as <scheme>::<field>::<tag>\n";
	}
};

struct DeltaCmd : ResourceCmd {
	DeltaCmd() : ResourceCmd("delta") { }

	virtual bool run(ShellSocketHandler &h, StringView &r) override {
		bool ret = false;
		auto schemeName = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();

		int64_t time = r.readInteger().get(0);
		auto schemeView = schemeName;
		StringView field; uint64_t tag = 0;
		schemeName = schemeView.readUntilString("::");
		if (schemeView.is("::")) {
			schemeView += 2;
			auto tmpField = schemeView.readUntilString("::");
			if (schemeView.is("::")) {
				schemeView += 2;
				auto tmpTag = schemeView.readInteger().get(0);
				if (tmpTag > 0 && !tmpField.empty()) {
					field = tmpField;
					tag = tmpTag;
				}
			}
		}

		if (auto s = acquireScheme(h, schemeName)) {
			h.performWithStorage([&] (const db::Transaction &t) {
				if (auto a = dynamic_cast<db::sql::SqlHandle *>(t.getAdapter().getBackendInterface())) {
					if (field.empty()) {
						h.sendData(a->getDeltaData(*s, Time::microseconds(time)));
					} else if (auto f = s->getField(field)) {
						if (f->getType() == db::Type::View) {
							h.sendData(a->getDeltaData(*s, *static_cast<const db::FieldView *>(f->getSlot()), Time::microseconds(time), tag));
						}
					}
					ret = true;
				}
			});
		}

		h.sendError(toString("Scheme is not defined"));
		return ret;
	}

	virtual StringView desc() const {
		return "<scheme> <time> [<field> <tag>] - Delta for scheme";
	}
	virtual StringView help() const {
		return "\tdelta <scheme|view> <time> - Changelog for scheme or view\n\n"
				"Scheme can be defined by it's name\n"
				"View can be defined as <scheme>::<field>::<tag>\n";
	}
};

struct MultiCmd : ResourceCmd {
	MultiCmd() : ResourceCmd("multi") { }

	virtual bool run(ShellSocketHandler &h, StringView &r) override {
		r.skipUntil<StringView::Chars<'('>>();
		if (r.is('(')) {
			Value result;
			Value requests = data::read<Interface>(r);
			if (requests.isDictionary()) {
				for (auto &it : requests.asDict()) {
					StringView path(it.first);
					StringView scheme = path.readUntil<StringView::Chars<'/'>>();
					if (path.is('/')) {
						++ path;
					}

					h.performWithStorage([&, this] (const db::Transaction &t) {
						if (auto r = acquireResource(t, h, scheme, path, StringView(), it.second)) {
							result.setValue(r->getResultObject(), it.first);
							delete r;
						}
					});
				}
			}
			h.sendData(result);
		}

		return true;
	}

	virtual StringView desc() const {
		return "<request> - perform multi-request";
	}
	virtual StringView help() const {
		return "<request> - perform multi-request";
	}
};

struct CreateCmd : ResourceCmd {
	CreateCmd() : ResourceCmd("create") { }

	virtual bool run(ShellSocketHandler &h, StringView &r) override {
		auto schemeName = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();

		StringView path("/");
		if (r.is('/') || r.is<StringView::CharGroup<CharGroupId::Numbers>>()) {
			path = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
			r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		}

		bool success = false;
		Value patch = (r.is('{') || r.is('[') || r.is('(')) ? data::read<Interface>(r) : UrlView::parseArgs<Interface>(r, 1_KiB);
		h.performWithStorage([&, this] (const db::Transaction &t) {
			if (auto r = acquireResource(t, h, schemeName, path, StringView())) {
				Vector<db::InputFile> f;
				if (r->prepareCreate()) {
					if (auto ret = r->createObject(patch, f)) {
						h.sendData(ret);
						success = true;
					}
				} else {
					h.sendError(toString("Action for scheme ", schemeName, " is forbidden for ", h.getUser()->getName()));
				}
				delete r;
			}
		});

		if (!success) {
			h.sendData(patch);
			h.sendError("Fail to create object with data:");
		}

		return true;
	}

	virtual StringView desc() const {
		return "<scheme> <path> <data> - Create object for scheme";
	}
	virtual StringView help() const {
		return "<scheme> <path> <data> - Create object for scheme";
	}
};

struct UpdateCmd : ResourceCmd {
	UpdateCmd() : ResourceCmd("update") { }

	virtual bool run(ShellSocketHandler &h, StringView &r) override {
		auto schemeName = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();

		StringView path("/");
		if (r.is('/') || r.is<StringView::CharGroup<CharGroupId::Numbers>>()) {
			path = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
			r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		}

		bool success = false;
		Value patch = (r.is('{') || r.is('[') || r.is('(')) ? data::read<Interface>(r) : UrlView::parseArgs<Interface>(r, 1_KiB);
		h.performWithStorage([&, this] (const db::Transaction &t) {
			if (auto r = acquireResource(t, h, schemeName, path, StringView())) {
				Vector<db::InputFile> f;
				if (r->prepareUpdate()) {
					if (auto ret = r->updateObject(patch, f)) {
						h.sendData(ret);
						success = true;
					}
				} else {
					h.sendError(toString("Action for scheme ", schemeName, " is forbidden for ", h.getUser()->getName()));
				}
				delete r;
			}
		});

		if (!success) {
			h.sendData(patch);
			h.sendError("Fail to update object with data:");
		}

		return true;
	}

	virtual StringView desc() const {
		return "<scheme> <path> <data> - Update object for scheme";
	}
	virtual StringView help() const {
		return "<scheme> <path> <data>  - Update object for scheme";
	}
};

struct UploadCmd : ResourceCmd {
	UploadCmd() : ResourceCmd("upload") { }

	virtual bool run(ShellSocketHandler &h, StringView &r) override {
		auto schemeName = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();

		StringView path("/");
		if (r.is('/') || r.is<StringView::CharGroup<CharGroupId::Numbers>>()) {
			path = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
			r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		}

		bool success = false;
		h.performWithStorage([&, this] (const db::Transaction &t) {
			if (auto r = acquireResource(t, h, schemeName, path, StringView())) {
				if (r->prepareCreate()) {
					Bytes bkey = valid::makeRandomBytes<Interface>(8);
					String key = base16::encode<Interface>(bkey);

					Value token({
						pair("scheme", Value(schemeName)),
						pair("path", Value(path)),
						pair("resolve", Value("")),
						pair("user", Value(h.getUser()->getObjectId())),
					});

					h.performWithStorage([&] (const db::Transaction &t) {
						t.getAdapter().set(key, token, TimeInterval::seconds(5));
					});

					auto tv = Time::now().toMicros();
					StringStream str;
					str << "<p id=\"tmp" << tv << "\"><input type=\"file\" name=\"content\" onchange=\""
								"upload(this.files[0], '" << toString(h.getUrl(), "/upload/", key) << "', 'content');"
								"var elem = document.getElementById('tmp" << tv << "');elem.parentNode.removeChild(elem);"
							"\"></p>";

					h.send(str.str());
					success = true;
				}
				delete r;
			}
		});

		if (!success) {
			h.sendError("Fail to prepare upload");
		}
		return true;
	}

	virtual StringView desc() const {
		return "<scheme> <path> - Upload file for scheme resource";
	}
	virtual StringView help() const {
		return "<scheme> <path> - Update file for scheme resource";
	}
};

struct AppendCmd : ResourceCmd {
	AppendCmd() : ResourceCmd("append") { }

	virtual bool run(ShellSocketHandler &h, StringView &r) override {
		auto schemeName = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();

		StringView path("/");
		if (r.is('/') || r.is<StringView::CharGroup<CharGroupId::Numbers>>()) {
			path = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
			r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		}

		bool success = false;
		Value patch = (r.is('{') || r.is('[') || r.is('(')) ? data::read<Interface>(r) : UrlView::parseArgs<Interface>(r, 1_KiB);
		h.performWithStorage([&, this] (const db::Transaction &t) {
			if (auto r = acquireResource(t, h, schemeName, path, StringView())) {
				if (r->prepareAppend()) {
					if (auto ret = r->appendObject(patch)) {
						h.sendData(ret);
						success = true;
					}
				} else {
					h.sendError(toString("Action for scheme ", schemeName, " is forbidden for ", h.getUser()->getName()));
				}
				delete r;
			}
		});

		if (!success) {
			h.sendData(patch);
			h.sendError("Fail to update object with data:");
		}

		return true;
	}

	virtual StringView desc() const {
		return "<scheme> <path> <data> - Append object for scheme";
	}
	virtual StringView help() const {
		return "<scheme> <path> <data> - Append object for scheme";
	}
};

struct DeleteCmd : ResourceCmd {
	DeleteCmd() : ResourceCmd("delete") { }

	virtual bool run(ShellSocketHandler &h, StringView &r) override {
		auto schemeName = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();

		StringView path("/");
		if (r.is('/') || r.is<StringView::CharGroup<CharGroupId::Numbers>>()) {
			path = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
			r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		}

		bool success = false;
		h.performWithStorage([&, this] (const db::Transaction &t) {
			if (auto r = acquireResource(t, h, schemeName, path, StringView())) {
				if (r->removeObject()) {
					success = true;
					h.sendData(Value(true));
				} else {
					h.sendError(toString("Action for scheme ", schemeName, " is forbidden for ", h.getUser()->getName()));
				}
				delete r;
			}
		});

		if (!success) {
			h.sendError("Fail to delete object");
		}

		return true;
	}

	virtual StringView desc() const {
		return "<scheme> <path> - Delete object for scheme";
	}
	virtual StringView help() const {
		return "<scheme> <path> - Delete object for scheme";
	}
};

struct SearchCmd : ResourceCmd {
	SearchCmd() : ResourceCmd("search") { }

	virtual bool run(ShellSocketHandler &h, StringView &r) override {
		auto schemeName = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();

		StringView path("/");
		if (r.is('/') || r.is<StringView::CharGroup<CharGroupId::Numbers>>()) {
			path = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
			r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		}
		Value data;
		if (r.is('(')) {
			data = data::serenity::read<Interface>(r);
		}

		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		if (!r.empty()) {
			data.setString(r, "search");
		}

		h.performWithStorage([&, this] (const db::Transaction &t) {
			if (auto res = acquireResource(t, h, schemeName, path, StringView(), data)) {
				if (auto val = res->getResultObject()) {
					h.sendData(val);
				} else {
					h.sendError(toString(schemeName, ": nothing is found"));
				}
				delete res;
			}
		});

		h.sendError("Fail run search");

		return true;
	}

	virtual StringView desc() const {
		return "<scheme> <path> <text> - Run full-text search";
	}
	virtual StringView help() const {
		return "<scheme> <path> <text> - Run full-text search";
	}
};

struct HandlersCmd : SocketCommand {
	HandlersCmd() : SocketCommand("handlers") { }

	virtual bool run(ShellSocketHandler &h, StringView &r) override {
		auto serv = h.manager()->host();
		auto &hdl = serv.getRequestHandlers();

		Value ret;
		for (auto &it : hdl) {
			auto &v = ret.emplace(it.first);
			if (!it.second.data.isNull()) {
				v.setValue(it.second.data, "data");
			}
			if (it.second.scheme) {
				v.setString(it.second.scheme->getName(), "scheme");
			}
			if (!it.second.component.empty()) {
				v.setString(it.second.component, "component");
			}
			if (it.first.back() == '/') {
				v.setBool(true, "forSubPaths");
			}
		}

		h.sendData(ret);
		return true;
	}

	virtual StringView desc() const {
		return " - Information about registered handlers";
	}
	virtual StringView help() const {
		return " - Information about registered handlers";
	}
};

struct CloseCmd : SocketCommand {
	CloseCmd() : SocketCommand("exit") { }

	virtual bool run(ShellSocketHandler &h, StringView &r) override {
		h.send("Connection will be closed by your request");
		return false;
	}

	virtual StringView desc() const {
		return " - close current connection";
	}
	virtual StringView help() const {
		return " - close current connection";
	}
};

struct EchoCmd : SocketCommand {
	EchoCmd() : SocketCommand("echo") { }

	virtual bool run(ShellSocketHandler &h, StringView &r) override {
		if (!r.empty()) { h.send(r); }
		return true;
	}

	virtual StringView desc() const {
		return "<message> - display message in current terminal";
	}
	virtual StringView help() const {
		return "<message> - display message in current terminal";
	}
};

struct ParseCmd : SocketCommand {
	ParseCmd() : SocketCommand("parse") { }

	virtual bool run(ShellSocketHandler &h, StringView &r) override {
		Value patch = (r.is('{') || r.is('[') || r.is('(')) ? data::read<Interface>(r) : UrlView::parseArgs<Interface>(r, 1_KiB);
		h.sendData(patch);
		return true;
	}

	virtual StringView desc() const {
		return "<message> - parse message as object changeset";
	}
	virtual StringView help() const {
		return "<message> - parse message as object changeset";
	}
};

struct MsgCmd : SocketCommand {
	MsgCmd() : SocketCommand("msg") { }

	virtual bool run(ShellSocketHandler &h, StringView &r) override {
		h.sendBroadcast(Value({
			std::make_pair("user", Value(h.getUser()->getName())),
			std::make_pair("event", Value("message")),
			std::make_pair("message", Value(r)),
		}));
		return true;
	}

	virtual StringView desc() const {
		return "<message> - display message in all opened terminals";
	}
	virtual StringView help() const {
		return "<message> - display message in all opened terminals";
	}
};

struct CountCmd : SocketCommand {
	CountCmd() : SocketCommand("count") { }

	virtual bool run(ShellSocketHandler &h, StringView &r) override {
		StringStream resp;
		resp << "Users on socket: " << h.manager()->size();
		h.send(resp.weak());
		return true;
	}

	virtual StringView desc() const {
		return " - display number of opened terminals";
	}
	virtual StringView help() const {
		return " - display number of opened terminals";
	}
};

struct HelpCmd : SocketCommand {
	HelpCmd() : SocketCommand("help") { }

	virtual bool run(ShellSocketHandler &h, StringView &r) override {
		auto & cmds = h.getCommands();
		auto & externals = h.getExternals();
		StringStream stream;
		if (r.empty()) {
			stream << "Loaded components:\n";
			for (auto &it : h.manager()->host().getComponents()) {
				stream << "  " << it.second->getName() << " - " << it.second->getVersion() << "\n";
			}

			stream << "Available commands:\n";
			for (auto &it : cmds) {
				stream << "  - " << it->name << " " << it->desc() << "\n";
			}

			for (auto &it : externals) {
				if (it.second && !it.second->empty()) {
					stream << " From component: " << it.first << "\n";
					for (auto &eit : *it.second) {
						stream << "  - " << eit.second.name << " " << eit.second.desc << "\n";
					}
				}
			}
		} else {
			bool found = false;
			for (auto &it : cmds) {
				if (r == it->name) {
					stream << "  - " << it->name << " " << it->desc() << "\n" << it->help();
					found = true;
					break;
				}
			}

			if (!found) {
				for (auto &it : externals) {
					stream << " From component: " << it.first << "\n";
					for (auto &eit : *it.second) {
						if (r == eit.second.name) {
							stream << "  - " << eit.second.name << " " << eit.second.desc << "\n" << eit.second.help;
							found = true;
							break;
						}
					}
					if (found) {
						break;
					}
				}
			}
		}
		h.send(stream.weak());
		return true;
	}

	virtual StringView desc() const {
		return "<>|<cmd> - display command list or information about command";
	}
	virtual StringView help() const {
		return "<>|<cmd> - display command list or information about command";
	}
};

struct GenPasswordCmd : SocketCommand {
	GenPasswordCmd() : SocketCommand("generate_password") { }

	virtual bool run(ShellSocketHandler &h, StringView &r) override {
		auto len = r.readInteger().get(6);
		h.send(valid::generatePassword<Interface>(len));
		return true;
	}

	virtual StringView desc() const {
		return " - generate password with <length>";
	}
	virtual StringView help() const {
		return " - generate password with <length>";
	}
};

/*struct KillCmd : SocketCommand {
	KillCmd() : SocketCommand("kill") { }

	virtual bool run(ShellSocketHandler &h, StringView &r) override {
		::raise(SIGSEGV);
		return true;
	}

	virtual StringView desc() const {
		return " - kill current process";
	}
	virtual StringView help() const {
		return " - kill current process";
	}
};*/

struct TimeCmd : SocketCommand {
	TimeCmd() : SocketCommand("time") { }

	virtual bool run(ShellSocketHandler &h, StringView &r) override {
		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		int64_t t = r.readInteger().get(0);
		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();

		if (t) {
			if (r == "s" || r == "sec") {
				h.send(toString(t, " sec -> ", Time::seconds(t).toHttp<Interface>()));
			} else if (r == "ms" || r == "msec") {
				h.send(toString(t, " ms -> ", Time::milliseconds(t).toHttp<Interface>()));
			} else if (r == "mcs" || r == "μs" || r == "mcsec" || r.empty()) {
				h.send(toString(t, " μs -> ", Time::microseconds(t).toHttp<Interface>()));
			} else {
				h.send("Invalid input");
			}
		} else if (r.empty()) {
			auto now = Time::now();
			h.send(toString(now.toMicros(), " μs -> ", now.toHttp<Interface>()));
		} else {
			h.send("Invalid input");
		}

		return true;
	}

	virtual StringView desc() const {
		return " - convert integer timestamp to human-readable format";
	}
	virtual StringView help() const {
		return " - convert integer timestamp to human-readable format.\n"
				"    <int>s | <int>sec - as seconds\n"
				"    <int>ms | <int>msec - as milliseconds\n"
				"    <int>mcs | <int>mcsec | <int> (no suffix) - as microseconds";
	}
};

/*struct StatCmd : SocketCommand {
	StatCmd() : SocketCommand("stat") { }

	virtual bool run(ShellSocketHandler &h, StringView &r) override {
		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();

		StringStream ret;

		auto root = Root::getCurrent();
		auto stat = root->getStat();

		if (r.starts_with("0x")) {
			ret << root->getAllocatorMemoryMap(r.readInteger().get()) << "\n";
		} else {
			ret << "Resource stat:\n";
			ret << "\tRequests recieved: " << stat.requestsRecieved << "\n";
			ret << "\tFilters init: " << stat.filtersInit << "\n";
			ret << "\tTasks runned: " << stat.tasksRunned << "\n";
			ret << "\tHeartbeat counter: " << stat.heartbeatCounter << "\n";
			ret << "\tDB queries performed: " << stat.dbQueriesPerformed << " (" << stat.dbQueriesReleased << " " << stat.dbQueriesPerformed - stat.dbQueriesReleased << ")\n";
			ret << "\n";

			ret << root->getMemoryMap(r == "full");
		}

		h.send(ret.str());
		return true;
	}

	virtual StringView desc() const {
		return " - show server instance statistics (current instance only)";
	}
	virtual StringView help() const {
		return " - server instance statistics (current instance only)\n"
				"    full - view all allocators with slots\n"
				"    <allocator-name> - allocator statistics with memory owners and backtraces";
	}
};*/

ShellSocketHandler::ShellSocketHandler(WebsocketManager *m, pool_t *pool, StringView url, int64_t userId)
: WebsocketHandler(m, pool, url, 600_sec), _userId(userId) {
	_cmds.push_back(new ListCmd());
	_cmds.push_back(new HandlersCmd());
	_cmds.push_back(new HistoryCmd());
	_cmds.push_back(new DeltaCmd());
	_cmds.push_back(new GetCmd());
	_cmds.push_back(new MultiCmd());
	_cmds.push_back(new CreateCmd());
	_cmds.push_back(new UpdateCmd());
	_cmds.push_back(new AppendCmd());
	_cmds.push_back(new UploadCmd());
	_cmds.push_back(new DeleteCmd());
	_cmds.push_back(new SearchCmd());
	//_cmds.push_back(new ModeCmd());
	_cmds.push_back(new DebugCmd());
	_cmds.push_back(new CloseCmd());
	_cmds.push_back(new EchoCmd());
	_cmds.push_back(new ParseCmd());
	_cmds.push_back(new MsgCmd());
	_cmds.push_back(new CountCmd());
	_cmds.push_back(new HelpCmd());
	_cmds.push_back(new GenPasswordCmd());
	//_cmds.push_back(new KillCmd());
	_cmds.push_back(new TimeCmd());
	//_cmds.push_back(new StatCmd());

	auto serv = m->host();
	_external.reserve(serv.getComponents().size());
	for (auto &it : serv.getComponents()) {
		if (!it.second->getCommands().empty()) {
			_external.emplace_back(StringView(it.first), &it.second->getCommands());
		}
	}
}

bool ShellSocketHandler::onCommand(StringView &r) {
	auto cmd = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
	r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();

	for (auto &it : _cmds) {
		if (cmd == it->name) {
			return it->run(*this, r);
		}
	}

	for (auto &it : _external) {
		for (auto &eit : *it.second) {
			if (cmd == eit.second.name) {
				bool ret = false;
				performWithStorage([&, this] (const db::Transaction &t) {
					t.setRole(db::AccessRoleId::Admin);
					ret = eit.second.callback(r, [&, this] (const Value &val) {
						send(val);
					});
				});
				return ret;
			}
		}
	}

	sendError("No such command, use 'help' to list available commands");
	return true;
}

void ShellSocketHandler::handleBegin() {
	performWithStorage([&, this] (const db::Transaction &t) {
		web::perform([&, this] {
			_user = db::User::get(t.getAdapter(), _userId);
		}, _pool);
	});

	sendBroadcast(Value({
		std::make_pair("user", Value(_user->getName())),
		std::make_pair("event", Value("enter")),
	}));

	StringStream resp;
	resp << "Serenity: Welcome. " << _user->getName() << "!";
	send(resp.weak());

	for (auto &it : _cmds) {
		if ("help" == it->name) {
			StringView r;
			it->run(*this, r);
			break;
		}
	}
}

bool ShellSocketHandler::handleFrame(WebsocketFrameType t, const Bytes &b) {
	if (t == WebsocketFrameType::Text) {
		StringView r((const char *)b.data(), b.size());
		sendCmd(r);
		return onCommand(r);
	}
	return true;
}

bool ShellSocketHandler::handleMessage(const Value &val) {
	if (val.isBool("message") && val.getBool("message")) {
		if (auto &d = val.getValue("data")) {
			if (d.getString("source") == "Database-Query") {
				StringStream resp;
				resp << " - [Query] " << d.getString("text") << "\n";
				send(resp.weak());
			} else {
				sendData(d);
			}
		}
	} else  if (val.isString("event")) {
		auto &ev = val.getString("event");
		if (ev == "enter") {
			StringStream resp;
			resp << "User " << val.getString("user") << " connected.";
			send(resp.weak());
		} else if (ev == "message") {
			StringStream resp;
			resp << "- [" << val.getString("user") << "] " << val.getString("message");
			send(resp.weak());
		}
	}

	return true;
}

void ShellSocketHandler::sendCmd(const StringView &v) {
	StringStream stream;
	stream << _user->getName() << "@" << _manager->host().getHostInfo().hostname << ": " << v;
	send(stream.weak());
}

void ShellSocketHandler::sendError(const String &str) {
	StringStream stream;
	switch(_mode) {
	case ShellMode::Plain: stream << "Error: " << str << "\n"; break;
	case ShellMode::Html: stream << "<span class=\"error\">Error:</span> " << str; break;
	};
	send(stream.weak());
}

void ShellSocketHandler::sendData(const Value & data) {
	StringStream stream;
	switch(_mode) {
	case ShellMode::Plain: data::write(stream, data, data::EncodeFormat::Json); break;
	case ShellMode::Html: stream << "<p>"; output::formatJsonAsHtml(stream, data); stream << "</p>"; break;
	};
	send(stream.weak());
}


WebsocketHandler * ShellSocket::onAccept(const Request &req, pool_t *pool) {
	WebsocketHandler *ret = nullptr;
	Request rctx(req);
	if (!req.getController()->isSecureAuthAllowed()) {
		rctx.setStatus(HTTP_FORBIDDEN);
		return nullptr;
	}

	if (auto user = req.getAuthorizedUser()) {
		if (!user || !user->isAdmin()) {
			rctx.setStatus(HTTP_FORBIDDEN);
		} else {
			web::perform([&, this] {
				ret = new ShellSocketHandler(this, pool, req.getInfo().url.path, user->getObjectId());
			}, pool);
		}
	}

	if (!ret) {
		auto &data = rctx.getInfo().queryData;
		if (data.isString("name") && data.isString("passwd")) {
			auto &name = data.getString("name");
			auto &passwd = data.getString("passwd");

			rctx.performWithStorage([&] (const db::Transaction &t) {
				db::User * user = db::User::get(t, name, passwd);
				if (!user || !user->isAdmin()) {
					rctx.setStatus(HTTP_FORBIDDEN);
				} else {
					rctx.setUser(user);
					web::perform([&, this] {
						ret = new ShellSocketHandler(this, pool, req.getInfo().url.path, user->getObjectId());
					}, pool);
				}
				return true;
			});
		}
	}
	return ret;
}

bool ShellSocket::onBroadcast(const Value &) {
	return true;
}

Status ShellGui::onPostReadRequest(Request &rctx) {
	auto &info = rctx.getInfo();
	if (info.method == RequestMethod::Get) {
		if (_subPath.empty()) {
			auto userScheme = rctx.host().getUserScheme();
			size_t count = 0;
			bool hasDb = false;
			if (userScheme) {
				rctx.performWithStorage([&] (const db::Transaction &t) {
					count = userScheme->count(t, db::Query());
					hasDb = true;
					return true;
				});
			}

			rctx.setContentType("text/html;charset=UTF-8");
			rctx.runPug("virtual://html/shell.pug", [&] (pug::Context &exec, const pug::Template &) -> bool {
				exec.set("count", Value(count));
				exec.set("setup", Value(count != 0));
				exec.set("hasDb", Value(hasDb));
				exec.set("version", Value(config::getWebserverVersionString()));
				return true;
			});

			return DONE;
		} else {
			return HTTP_NOT_FOUND;
		}
	} else if (info.method == RequestMethod::Post) {
		StringView path(_subPath);
		if (!path.is("/upload/")) {
			return HTTP_NOT_IMPLEMENTED;
		} else {
			path += "/upload/"_len;
			Status status = HTTP_NOT_FOUND;
			rctx.performWithStorage([&, this] (const db::Transaction &t) {
				auto data = t.getAdapter().get(path);
				if (!path.empty()) {
					if (data) {
						t.getAdapter().clear(path);
					} else {
						status = HTTP_NOT_FOUND;
						return false;
					}

					auto scheme = rctx.host().getScheme(data.getString("scheme"));
					auto path = data.getString("path");
					auto resolve = data.getString("resolve");
					auto user = db::User::get(t, data.getInteger("user"));
					if (scheme && !path.empty() && user) {
						if (Resource *r = Resource::resolve(t, *scheme, path)) {
							r->setUser(user);
							if (!resolve.empty()) {
								r->setResolveOptions(Value(resolve));
							}
							if (r->prepareCreate()) {
								_resource = r;
								status = DECLINED;
							}
						}
					}
				}
				return true;
			});
			return status;
		}
		return HTTP_NOT_FOUND;
	}
	return DECLINED;
}

void ShellGui::onInsertFilter(Request &rctx) {
	if (!_resource) {
		return;
	}

	rctx.setInputConfig(db::InputConfig({
		db::InputConfig::Require::Files,
		_resource->getMaxRequestSize(),
		_resource->getMaxVarSize(),
		_resource->getMaxFileSize()
	}));

	auto &info = rctx.getInfo();
	if (info.method == RequestMethod::Put || info.method == RequestMethod::Post) {
		auto ex = InputFilter::insert(rctx);
		if (ex != InputFilter::Exception::None) {
			if (ex == InputFilter::Exception::TooLarge) {
				rctx.setStatus(HTTP_REQUEST_ENTITY_TOO_LARGE);
			} else if (ex == InputFilter::Exception::Unrecognized) {
				rctx.setStatus(HTTP_UNSUPPORTED_MEDIA_TYPE);
			}
		}
	}
}

Status ShellGui::onHandler(Request &) {
	return OK;
}

void ShellGui ::onFilterComplete(InputFilter *filter) {
	Request rctx(filter->getRequest());
	Value data;
	data.setBool(false, "OK");
	if (_resource) {
		_request.performWithStorage([&, this] (const db::Transaction &t) {
			return t.performAsSystem([&, this] {
				data.setValue(_resource->createObject(filter->getData(), filter->getFiles()), "result");
				data.setBool(true, "OK");
				return true;
			});
		});
	}

	data.setInteger(Time::now().toMicros(), "date");
#if DEBUG
	auto &debug = _request.getDebugMessages();
	if (!debug.empty()) {
		data.setArray(debug, "debug");
	}
#endif
	auto &error = _request.getErrorMessages();
	if (!error.empty()) {
		data.setArray(error, "errors");
	}

	_request.writeData(data, false);
}

}
