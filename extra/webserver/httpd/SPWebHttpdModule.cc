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

#include "SPWebHttpdRoot.h"
#include "SPWebHttpdHost.h"
#include "SPWebHttpdRequest.h"
#include "SPWebHttpdFilters.h"
#include "SPWebHost.h"

#include "ap_provider.h"
#include "mod_log_config.h"
#include "mod_auth.h"


static stappler::web::HttpdRoot *s_sharedRoot;

extern module AP_MODULE_DECLARE_DATA stappler_web_module;

namespace STAPPLER_VERSIONIZED stappler::web {

static constexpr auto CompressFilterName = "stappler::web::httpd::Compress";

static void *mod_stappler_web_httpd_create_server_config(apr_pool_t *p, server_rec *s) {
	if (!s_sharedRoot) {
		auto rootPool = reinterpret_cast<pool_t *>(s->process->pool);
		perform([&] {
			s_sharedRoot = new (rootPool) HttpdRoot(rootPool);
		}, rootPool);
	}

	return HttpdHostController::create(s_sharedRoot, s);
}

static void* mod_stappler_web_httpd_merge_config(apr_pool_t* pool, void* BASE, void* ADD) {
	return perform([&] () -> auto {
		return HttpdHostController::merge((HttpdHostController *)BASE, (HttpdHostController *)ADD);
	}, reinterpret_cast<pool_t *>(pool));
}

static int mod_stappler_web_httpd_post_config(apr_pool_t *p, apr_pool_t *plog, apr_pool_t *ptemp, server_rec *s) {
	return perform([&] () -> int {
		return s_sharedRoot->handlePostConfig(reinterpret_cast<pool_t *>(p), s);
	}, reinterpret_cast<pool_t *>(p));
}
static void mod_stappler_web_httpd_child_init(apr_pool_t *p, server_rec *s) {
	return perform([&] () {
		s_sharedRoot->handleChildInit(reinterpret_cast<pool_t *>(p), s);
	}, reinterpret_cast<pool_t *>(p));
}

static int mod_stappler_web_httpd_check_access_ex(request_rec *r) {
	auto req = Request(HttpdRequestController::get(r));
	return s_sharedRoot->runCheckAccess(req);
}
static int mod_stappler_web_httpd_type_checker(request_rec *r) {
	auto req = Request(HttpdRequestController::get(r));
	return s_sharedRoot->runTypeChecker(req);
}
static int mod_stappler_web_httpd_post_read_request(request_rec *r) {
	ap_add_output_filter(CompressFilterName, nullptr, r, r->connection);

	auto req = Request(HttpdRequestController::get(r));

	HttpdOutputFilter::insert(req);

	return s_sharedRoot->runPostReadRequest(req);
}
static int mod_stappler_web_httpd_translate_name(request_rec *r) {
	auto req = Request(HttpdRequestController::get(r));
	return s_sharedRoot->runTranslateName(req);
}
static int mod_stappler_web_httpd_quick_handler(request_rec *r, int v) {
	auto req = Request(HttpdRequestController::get(r));
	return s_sharedRoot->runQuickHandler(req, v);
}
static void mod_stappler_web_httpd_insert_filter(request_rec *r) {
	auto req = Request(HttpdRequestController::get(r));
	s_sharedRoot->runInsertFilter(req);
}
static int mod_stappler_web_httpd_handler(request_rec *r) {
	auto req = Request(HttpdRequestController::get(r));
	return s_sharedRoot->runHandler(req);
}

static apr_status_t mod_stappler_web_httpd_compress(ap_filter_t *f, apr_bucket_brigade *bb) {
	return perform([&] () -> int {
		return compress_filter(f, bb);
	}, reinterpret_cast<pool_t *>(f->r->pool));
}

static void mod_stappler_web_httpd_register_hooks(apr_pool_t *pool) {
    ap_hook_post_config(mod_stappler_web_httpd_post_config, NULL,NULL,APR_HOOK_MIDDLE);
	ap_hook_child_init(mod_stappler_web_httpd_child_init, NULL, NULL, APR_HOOK_MIDDLE);

    ap_hook_check_access_ex(mod_stappler_web_httpd_check_access_ex, NULL, NULL, APR_HOOK_FIRST, AP_AUTH_INTERNAL_PER_URI);

    ap_hook_type_checker(mod_stappler_web_httpd_type_checker,NULL,NULL,APR_HOOK_FIRST);

	ap_hook_post_read_request(mod_stappler_web_httpd_post_read_request, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_translate_name(mod_stappler_web_httpd_translate_name, NULL, NULL, APR_HOOK_LAST);
	ap_hook_quick_handler(mod_stappler_web_httpd_quick_handler, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_insert_filter(mod_stappler_web_httpd_insert_filter, NULL, NULL, APR_HOOK_LAST);
    ap_hook_handler(mod_stappler_web_httpd_handler, NULL, NULL, APR_HOOK_MIDDLE);

    ap_register_output_filter(CompressFilterName, mod_stappler_web_httpd_compress, NULL, AP_FTYPE_CONTENT_SET);
}

static const char *mod_stappler_web_httpd_set_source_root(cmd_parms *parms, void *mconfig, const char *w) {
	auto host = Host(HttpdHostController::get(parms->server));
	perform([&] {
		host.addSourceRoot(StringView(w));
	}, reinterpret_cast<pool_t *>(parms->pool), config::TAG_HOST, host.getController());
	return NULL;
}

static const char *mod_stappler_web_httpd_add_component(cmd_parms *parms, void *mconfig, const char *w) {
	auto host = Host(HttpdHostController::get(parms->server));
	perform([&] {
		host.addComponentByParams(StringView(w));
	}, reinterpret_cast<pool_t *>(parms->pool), config::TAG_HOST, host.getController());
	return NULL;
}

static const char *mod_stappler_web_httpd_add_allow(cmd_parms *parms, void *mconfig, const char *w) {
	auto host = Host(HttpdHostController::get(parms->server));
	perform([&] {
		host.addAllow(StringView(w));
	}, reinterpret_cast<pool_t *>(parms->pool), config::TAG_HOST, host.getController());
	return NULL;
}

static const char *mod_stappler_web_httpd_set_session_params(cmd_parms *parms, void *mconfig, const char *w) {
	auto host = Host(HttpdHostController::get(parms->server));
	perform([&] {
		host.setSessionParams(StringView(w));
	}, reinterpret_cast<pool_t *>(parms->pool), config::TAG_HOST, host.getController());
	return NULL;
}

static const char *mod_stappler_web_httpd_set_host_secret(cmd_parms *parms, void *mconfig, const char *w) {
	auto host = Host(HttpdHostController::get(parms->server));
	perform([&] {
		host.setHostSecret(StringView(w));
	}, reinterpret_cast<pool_t *>(parms->pool), config::TAG_HOST, host.getController());
	return NULL;
}

static const char *mod_stappler_web_httpd_set_root_threads_count(cmd_parms *parms, void *mconfig, const char *w1, const char *w2) {
	perform([&] {
		s_sharedRoot->setThreadsCount(StringView(w1), StringView(w2));
	}, reinterpret_cast<pool_t *>(parms->pool));
	return NULL;
}

static const char *mod_stappler_web_httpd_set_force_https(cmd_parms *parms, void *mconfig) {
	auto host = Host(HttpdHostController::get(parms->server));
	perform([&] {
		host.setForceHttps();
	}, reinterpret_cast<pool_t *>(parms->pool), config::TAG_HOST, host.getController());
	return NULL;
}

static const char *mod_stappler_web_httpd_set_protected(cmd_parms *parms, void *mconfig, const char *w) {
	auto host = Host(HttpdHostController::get(parms->server));
	perform([&] {
		host.setProtectedList(StringView(w));
	}, reinterpret_cast<pool_t *>(parms->pool), config::TAG_HOST, host.getController());
	return NULL;
}

static const char *mod_stappler_web_httpd_set_server_names(cmd_parms *parms, void *mconfig, const char *arg) {
    if (!parms->server->names) {
        return "Only used in <VirtualHost>";
    }

    bool hostname = false;
	while (*arg) {
		char **item, *name = ap_getword_conf(parms->pool, &arg);
		if (!hostname) {
			parms->server->server_hostname = apr_pstrdup(parms->pool, name);
			hostname = true;
		} else {
			if (ap_is_matchexp(name)) {
				item = (char **) apr_array_push(parms->server->wild_names);
			} else {
				item = (char **) apr_array_push(parms->server->names);
			}
			*item = name;
		}
	}

    return NULL;
}

static const char *mod_stappler_web_httpd_set_root_db_params(cmd_parms *parms, void *mconfig, const char *w) {
	if (parms->server->is_virtual) {
		return NULL;
	}

	perform([&] {
		s_sharedRoot->setDbParams(StringView(w));
	}, reinterpret_cast<pool_t *>(parms->pool));
	return NULL;
}

static const char *mod_stappler_web_httpd_add_create_db(cmd_parms *parms, void *mconfig, const char *arg) {
	if (parms->server->is_virtual) {
		return NULL;
	}

	perform([&] {
		while (*arg) {
			char *name = ap_getword_conf(parms->pool, &arg);
			s_sharedRoot->addDb(StringView(name));
		}
	}, reinterpret_cast<pool_t *>(parms->pool));
	return NULL;
}

static const char *mod_stappler_web_httpd_set_db_params(cmd_parms *parms, void *mconfig, const char *w) {
	auto host = Host(HttpdHostController::get(parms->server));
	perform([&] {
		host.setDbParams(StringView(w));
	}, reinterpret_cast<pool_t *>(parms->pool), config::TAG_HOST, host.getController());
	return NULL;
}

static const command_rec mod_stappler_web_httpd_directives[] = {
	AP_INIT_TAKE1("StapplerSourceRoot", (cmd_func)mod_stappler_web_httpd_set_source_root, NULL, RSRC_CONF,
		"Root dir, where to search DSO for components"),

	AP_INIT_RAW_ARGS("StapplerComponent", (cmd_func)mod_stappler_web_httpd_add_component, NULL, RSRC_CONF,
		"Host component definition in format (Name:File:Func Args)"),

	AP_INIT_RAW_ARGS("StapplerSession", (cmd_func)mod_stappler_web_httpd_set_session_params, NULL, RSRC_CONF,
		"Session params (name, key, host, maxage, secure)"),

	AP_INIT_TAKE1("StapplerHostSecret", (cmd_func)mod_stappler_web_httpd_set_host_secret, NULL, RSRC_CONF,
		"Security key for host, that will be used for cryptographic proposes"),

	AP_INIT_NO_ARGS("StapplerForceHttps", (cmd_func)mod_stappler_web_httpd_set_force_https, NULL, RSRC_CONF,
		"Host should forward insecure requests to secure connection"),

	AP_INIT_RAW_ARGS("StapplerProtected", (cmd_func)mod_stappler_web_httpd_set_protected, NULL, RSRC_CONF,
		"Space-separated list of location prefixes, which should be invisible for clients"),

	AP_INIT_RAW_ARGS("StapplerServerNames", (cmd_func)mod_stappler_web_httpd_set_server_names, NULL, RSRC_CONF,
		"Space-separated list of server names (first would be ServerName, others - ServerAliases)"),

	AP_INIT_RAW_ARGS("StapplerAllowIp", (cmd_func)mod_stappler_web_httpd_add_allow, NULL, RSRC_CONF,
		"Additional IPv4 masks to thrust when admin access is requested"),

	AP_INIT_TAKE2("StapplerRootThreadsCount", (cmd_func)mod_stappler_web_httpd_set_root_threads_count, NULL, RSRC_CONF,
		"<init> <max> - size of root thread pool for async tasks"),

	AP_INIT_RAW_ARGS("StapplerRootDbParams", (cmd_func)mod_stappler_web_httpd_set_root_db_params, NULL, RSRC_CONF,
		"Database parameters for root connections (driver, host, dbname, user, password, other driver-defined params), has no effect in vhost"),

	AP_INIT_RAW_ARGS("StapplerRootCreateDb", (cmd_func)mod_stappler_web_httpd_add_create_db, NULL, RSRC_CONF,
		"Space-separated list of databases, that need to be created"),

	AP_INIT_RAW_ARGS("StapplerDbParams", (cmd_func)mod_stappler_web_httpd_set_db_params, NULL, RSRC_CONF,
		"Enable custom dbd connections for server with parameters (driver, host, dbname, user, password, other driver-defined params). "
		"Driver and parameters, that was not defined, inherited from StapplerRootDbParams. "
		"Parameter dbname will be automatically added to CreateDb list"),

    { NULL }
};

}

module AP_MODULE_DECLARE_DATA stappler_web_module = {
	STANDARD20_MODULE_STUFF,
	NULL, /* Per-directory configuration handler */
	NULL, /* Merge handler for per-directory configurations */
	stappler::web::mod_stappler_web_httpd_create_server_config, /* Per-server configuration handler */
	stappler::web::mod_stappler_web_httpd_merge_config, /* Merge handler for per-server configurations */
	stappler::web::mod_stappler_web_httpd_directives, /* Any directives we may have for httpd */
	stappler::web::mod_stappler_web_httpd_register_hooks /* Our hook registering function */
};
