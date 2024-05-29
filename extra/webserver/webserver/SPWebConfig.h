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

#ifndef EXTRA_WEBSERVER_WEBSERVER_SPWEBCONFIG_H_
#define EXTRA_WEBSERVER_WEBSERVER_SPWEBCONFIG_H_

#include "SPStringView.h"
#include "SPTime.h"

namespace STAPPLER_VERSIONIZED stappler::web::config {

constexpr auto HEARTBEAT_PAUSE = 1_sec;
constexpr auto HEARTBEAT_TIME = 1_sec;

#if DEBUG
constexpr auto DEFAULT_PUG_UPDATE_INTERVAL = 3_sec;
constexpr auto DEFAULT_DATABASE_CLEANUP_INTERVAL = 60_sec;
#else
constexpr auto DEFAULT_PUG_UPDATE_INTERVAL = 30_sec;
constexpr auto DEFAULT_DATABASE_CLEANUP_INTERVAL = 180_sec;
#endif

constexpr auto AUTH_MAX_TIME = 720_sec;

constexpr auto DEFAULT_SERVER_LINE = "StapplerWebserver/1.0";

constexpr auto FILE_SCHEME_NAME = "__files";
constexpr auto USER_SCHEME_NAME = "__users";
constexpr auto ERROR_SCHEME_NAME = "__error";

constexpr auto DEFAULT_SESSION_NAME = "SID";
constexpr auto DEFAULT_SESSION_KEY = "STAPPLER_SESSION_KEY";

constexpr uint32_t MAX_DB_CONNECTIONS = 1024;

constexpr int TAG_HOST = 1;
constexpr int TAG_CONNECTION = 2;
constexpr int TAG_REQUEST = 3;
constexpr int TAG_WEBSOCKET = 4;

constexpr auto WEBSOCKET_DEFAULT_TTL = 60_sec;
constexpr auto WEBSOCKET_DEFAULT_MAX_FRAME_SIZE = 1_KiB;

constexpr uint8_t PriorityLowest = 0;
constexpr uint8_t PriorityLow = 63;
constexpr uint8_t PriorityNormal = 127;
constexpr uint8_t PriorityHigh = 191;
constexpr uint8_t PriorityHighest = 255;

constexpr size_t MAX_INPUT_POST_SIZE = 2_GiB;
constexpr size_t MAX_INPUT_FILE_SIZE = 2_GiB;
constexpr size_t MAX_INPUT_VAR_SIZE =  8_KiB;

constexpr auto TOOLS_SERVER_PREFIX = StringView("/__server");
constexpr auto TOOLS_SHELL = StringView("/shell/");
constexpr auto TOOLS_SHELL_SOCKET = StringView("/shell");
constexpr auto TOOLS_ERRORS = StringView("/errors/");
constexpr auto TOOLS_AUTH = StringView("/auth/");
constexpr auto TOOLS_DOCS = StringView("/docs/");
constexpr auto TOOLS_HANDLERS = StringView("/handlers");
constexpr auto TOOLS_REPORTS = StringView("/reports/");
constexpr auto TOOLS_VIRTUALFS = StringView("/virtual/");

/* Этот ключ защищает хранимые в БД созданные автоматически ключи сервера
 * На его основе создаётся шифроблок, в котором хранятся созданные ключи
 * Второй ключ шифроблока - текстовый ключ HostSecret */
constexpr StringView INTERNAL_PRIVATE_KEY(
R"GostKey(-----BEGIN PRIVATE KEY-----
MGgCAQAwIQYIKoUDBwEBAQIwFQYJKoUDBwECAQIBBggqhQMHAQECAwRAzyYtvsjl
i2APv6u/DaGuR5t5tek8+rERoIE/hI9AuMu0JyIMo4Hd26NIMXsR9NODV+Nh8a9L
E8OCKW9N7veiLA==
-----END PRIVATE KEY-----
)GostKey");

constexpr auto DIR_MIME_TYPE = "httpd/unix-directory";

extern const char * MIME_TYPES;

const char * getWebserverVersionString();
uint32_t getWebserverVersionNumber();
uint32_t getWebserverVersionBuild();

}

#endif /* EXTRA_WEBSERVER_WEBSERVER_SPWEBCONFIG_H_ */
