#include "webServer.h"

#include <hash.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "WiFiManager.h"
#include "configManager.h"
#include "operations.h"
#include "hardware.h"
#include "homeKit2.h"
#include "logging.h"
#include "web.h"

typedef struct
{
	const char *Path;
	const unsigned char *Data;
	const size_t Size;
	const bool Zipped;
} StaticFilesMap;

const static StaticFilesMap staticFilesMap[] = {
	{"/login.html", login_html_gz , login_html_gz_len, true},
	{"/index.html", index_html_gz, index_html_gz_len, true},
	{"/media/logo.png", logo_png_gz, logo_png_gz_len, false},
	{"/media/favicon.png", favicon_png_gz, favicon_png_gz_len, false},
	{"/media/logout.png", logout_png_gz, logout_png_gz_len, false},
    {"/js/all.js", all_js, all_js_len, true},
	{"/css/mdb.min.css", mdb_min_css_gz, mdb_min_css_gz_len, true},
};

WebServer WebServer::instance;

static const char JsonMediaType[] PROGMEM = "application/json";

void WebServer::begin()
{
	LOG_DEBUG(F("WebServer Starting"));
	httpServer.begin();
	serverRouting();
	LOG_INFO(F("WebServer Started"));
}

bool WebServer::manageSecurity(AsyncWebServerRequest *request)
{
	if (!isAuthenticated(request))
	{
		LOG_WARNING(F("Auth Failed"));
		request->send(401, FPSTR(JsonMediaType), F("{\"msg\": \"Not-authenticated\"}"));
		return false;
	}
	return true;
}

void WebServer::serverRouting()
{
	httpServer.on(PSTR("/login.handler"), HTTP_POST, handleLogin);
	httpServer.on(PSTR("/logout.handler"), HTTP_GET, handleLogout);
	httpServer.on(PSTR("/wifiupdate.handler"), HTTP_POST, wifiUpdate);
	httpServer.on(PSTR("/factory.reset.handler"), HTTP_POST, factoryReset);
	httpServer.on(PSTR("/homekit.reset.handler"), HTTP_POST, homekitReset);
	httpServer.on(PSTR("/restart.handler"), HTTP_POST, restartDevice);
	httpServer.on(PSTR("/weblogin.update.handler"), HTTP_POST, webLoginUpdate);
	httpServer.on(PSTR("/othersettings.update.handler"), HTTP_POST, otherSettingsUpdate);
	httpServer.on(PSTR("/othersettings.update.handler"), HTTP_POST, otherSettingsUpdate);

	httpServer.on(PSTR("/api/sensor/get"), HTTP_GET, sensorGet);
	httpServer.on(PSTR("/api/wifi/get"), HTTP_GET, wifiGet);
	httpServer.on(PSTR("/api/information/get"), HTTP_GET, informationGet);
	httpServer.on(PSTR("/api/homekit/get"), HTTP_GET, homekitGet);
	httpServer.on(PSTR("/api/config/get"), HTTP_GET, configGet);

	httpServer.onNotFound(handleFileRead);
}

void WebServer::wifiGet(AsyncWebServerRequest *request)
{
	LOG_DEBUG(F("/api/wifi/get"));
	if (!manageSecurity(request))
	{
		return;
	}

	String JSON;
	StaticJsonDocument<200> jsonBuffer;

	jsonBuffer[F("captivePortal")] = WifiManager::instance.isCaptivePortal();
	jsonBuffer[F("ssid")] = WifiManager::SSID();
	serializeJson(jsonBuffer, JSON);

	request->send(200, FPSTR(JsonMediaType), JSON);
}

void WebServer::wifiUpdate(AsyncWebServerRequest *request)
{
	const auto SsidParameter = F("ssid");
	const auto PasswordParameter = F("wifipassword");

	LOG_INFO(F("Wifi Update"));

	if (!manageSecurity(request))
	{
		return;
	}

	if (request->hasArg(SsidParameter) && request->hasArg(PasswordParameter))
	{
		WifiManager::instance.setNewWifi(request->arg(SsidParameter), request->arg(PasswordParameter));
		redirectToRoot(request);
		return;
	}
	else
	{
		LOG_ERROR(F("Required parameters not provided"));
		request->send(400);
		return;
	}
}

template <class Array, class K, class T>
void addKeyValueObject(Array &array, const K &key, const T &value)
{
	auto j1 = array.createNestedObject();
	j1[F("key")] = key;
	j1[F("value")] = value;
}

void WebServer::informationGet(AsyncWebServerRequest *request)
{
	LOG_DEBUG(F("/api/information/get"));
	if (!manageSecurity(request))
	{
		return;
	}

	DynamicJsonDocument arr(1024);

	addKeyValueObject(arr, F("AP SSID"), WifiManager::SSID());
	addKeyValueObject(arr, F("AP Signal Strength"), WifiManager::RSSI());

	addKeyValueObject(arr, F("Reset Reason"), ESP.getResetReason());
	addKeyValueObject(arr, F("CPU Frequency (MHz)"), ESP.getCpuFreqMHz());

	addKeyValueObject(arr, F("Max Block Free Size (KB)"), ESP.getMaxFreeBlockSize() / 1024);
	addKeyValueObject(arr, F("Free Heap (KB)"), ESP.getFreeHeap() / 1024);

	FSInfo fsInfo;
	LittleFS.info(fsInfo);

	addKeyValueObject(arr, F("Filesystem Total Size (KB)"), fsInfo.totalBytes / 1024);
	addKeyValueObject(arr, F("Filesystem Free Size (KB)"), (fsInfo.totalBytes - fsInfo.usedBytes) / 1024);

	String JSON;
	serializeJson(arr, JSON);
	request->send(200, FPSTR(JsonMediaType), JSON);
}

void WebServer::homekitGet(AsyncWebServerRequest *request)
{
	LOG_DEBUG(F("/api/homekit/get"));
	if (!manageSecurity(request))
	{
		return;
	}

	DynamicJsonDocument arr(1024);

	const bool paired = homeKit2::instance.isPaired();

	addKeyValueObject(arr, F("Paired"), homeKit2::instance.isPaired() ? F("Yes") : F("No"));
	if (!paired)
	{
		addKeyValueObject(arr, F("Password"), homeKit2::instance.getPassword());
	}

	String JSON;
	serializeJson(arr, JSON);
	request->send(200, FPSTR(JsonMediaType), JSON);
}

void WebServer::configGet(AsyncWebServerRequest *request)
{
	LOG_DEBUG(F("/api/information/get"));
	if (!manageSecurity(request))
	{
		return;
	}
	const String json = config::instance.getAllConfigAsJson();
	request->send(200, FPSTR(JsonMediaType), json);
}

template <class T>
void addToJsonDoc(JsonDocument &doc, T id, float value)
{
	if (!isnan(value))
	{
		doc[id] = serialized(String(value, 2));
	}
}

void WebServer::sensorGet(AsyncWebServerRequest *request)
{
	LOG_DEBUG(F("/api/sensor/get"));
	if (!manageSecurity(request))
	{
		return;
	}

	StaticJsonDocument<256> doc;
	addToJsonDoc(doc, F("humidity"), hardware::instance.getHumidity());
	addToJsonDoc(doc, F("temperatureC"), hardware::instance.getTemperatureC());

	String buf;
	serializeJson(doc, buf);
	request->send(200, FPSTR(JsonMediaType), buf);
}

// Check if header is present and correct
bool WebServer::isAuthenticated(AsyncWebServerRequest *request)
{
	LOG_TRACE(F("Checking if authenticated"));
	if (request->hasHeader(F("Cookie")))
	{
		const String cookie = request->header(F("Cookie"));
		LOG_TRACE(F("Found cookie: ") << cookie);

		const String token = sha1(config::instance.data.webUserName + ":" +
								  config::instance.data.webPassword + ":" +
								  request->client()->remoteIP().toString());

		if (cookie.indexOf(F("ESPSESSIONID=") + token) != -1)
		{
			LOG_TRACE("Authentication Successful");
			return true;
		}
	}
	LOG_TRACE("Authentication Failed");
	return false;
}

void WebServer::handleLogin(AsyncWebServerRequest *request)
{
	const auto CookieHeader = F("Cookie");
	const auto UserNameParameter = F("username");
	const auto PasswordParameter = F("password");

	LOG_INFO(F("Handle login"));
	String msg;
	if (request->hasHeader(CookieHeader))
	{
		// Print cookies
		LOG_TRACE(F("Found cookie: ") << request->header(CookieHeader));
	}

	if (request->hasArg(UserNameParameter) && request->hasArg(PasswordParameter))
	{
		if (request->arg(UserNameParameter).equalsIgnoreCase(config::instance.data.webUserName) &&
			request->arg(PasswordParameter).equalsConstantTime(config::instance.data.webPassword))
		{
			LOG_INFO(F("User/Password correct"));
			auto response = request->beginResponse(301); // Sends 301 redirect

			response->addHeader(F("Location"), F("/"));
			response->addHeader(F("Cache-Control"), F("no-cache"));

			String token = sha1(config::instance.data.webUserName + F(":") + config::instance.data.webPassword +
								F(":") + request->client()->remoteIP().toString());
			LOG_DEBUG(F("Token: ") << token);
			response->addHeader(F("Set-Cookie"), F("ESPSESSIONID=") + token);

			request->send(response);
			LOG_INFO(F("Log in Successful"));
			return;
		}

		msg = F("Wrong username/password! Try again.");
		LOG_WARNING(F("Log in Failed"));
		auto response = request->beginResponse(301); // Sends 301 redirect

		response->addHeader(F("Location"), F("/login.html?msg=") + msg);
		response->addHeader(F("Cache-Control"), F("no-cache"));
		request->send(response);
		return;
	}
	else
	{
		LOG_ERROR(F("Login Parameter not provided"));
		request->send(400);
	}
}

/**
 * Manage logout (simply remove correct token and redirect to login form)
 */
void WebServer::handleLogout(AsyncWebServerRequest *request)
{
	LOG_INFO(F("Disconnection"));
	AsyncWebServerResponse *response = request->beginResponse(301); // Sends 301 redirect
	response->addHeader(F("Location"), F("/login.html?msg=User disconnected"));
	response->addHeader(F("Cache-Control"), F("no-cache"));
	response->addHeader(F("Set-Cookie"), F("ESPSESSIONID=0"));
	request->send(response);
	return;
}

void WebServer::webLoginUpdate(AsyncWebServerRequest *request)
{
	const auto webUserName = F("webUserName");
	const auto webPassword = F("webPassword");

	LOG_INFO(F("web login Update"));

	if (!manageSecurity(request))
	{
		return;
	}

	if (request->hasArg(webUserName) && request->hasArg("webPassword"))
	{
		LOG_INFO(F("Updating web username/password"));
		config::instance.data.webUserName = request->arg(webUserName);
		config::instance.data.webPassword = request->arg(webPassword);
	}
	else
	{
		LOG_WARNING(F("Correct Parameters not provided"));
		request->send(400);
	}

	config::instance.save();
	redirectToRoot(request);
}

void WebServer::otherSettingsUpdate(AsyncWebServerRequest *request)
{
	const auto hostName = F("hostName");
	const auto sensorsRefreshInterval = F("sensorsRefreshInterval");
	const auto showDisplayInF = F("showDisplayInF");

	LOG_INFO(F("config Update"));

	if (!manageSecurity(request))
	{
		return;
	}

	if (request->hasArg(hostName))
	{
		config::instance.data.hostName = request->arg(hostName);
	}

	if (request->hasArg(sensorsRefreshInterval))
	{
		config::instance.data.sensorsRefreshInterval = request->arg(sensorsRefreshInterval).toInt() * 1000;
	}

	if (request->hasArg(showDisplayInF))
	{
		config::instance.data.showDisplayInF = !request->arg(showDisplayInF).equalsIgnoreCase("on");
	}
	else
	{
		config::instance.data.showDisplayInF = false;
	}

	config::instance.save();
	redirectToRoot(request);
}

void WebServer::restartDevice(AsyncWebServerRequest *request)
{
	LOG_INFO(F("restart"));

	if (!manageSecurity(request))
	{
		return;
	}

	redirectToRoot(request);
	operations::instance.reboot();
}

void WebServer::factoryReset(AsyncWebServerRequest *request)
{
	LOG_WARNING(F("factoryReset"));

	if (!manageSecurity(request))
	{
		return;
	}

	redirectToRoot(request);
	operations::instance.factoryReset();
}

void WebServer::homekitReset(AsyncWebServerRequest *request)
{
	LOG_WARNING(F("homekitReset"));

	if (!manageSecurity(request))
	{
		return;
	}

	config::instance.data.homeKitPairData.resize(0);
	config::instance.save();
	redirectToRoot(request);
	operations::instance.reboot();
}

String WebServer::getContentType(const String &filename)
{
	if (filename.endsWith(F(".htm")))
		return F("text/html");
	else if (filename.endsWith(F(".html")))
		return F("text/html");
	else if (filename.endsWith(F(".css")))
		return F("text/css");
	else if (filename.endsWith(F(".js")))
		return F("application/javascript");
	else if (filename.endsWith(F(".json")))
		return F("application/json");
	else if (filename.endsWith(F(".png")))
		return F("image/png");
	else if (filename.endsWith(F(".gif")))
		return F("image/gif");
	else if (filename.endsWith(F(".jpg")))
		return F("image/jpeg");
	else if (filename.endsWith(F(".jpeg")))
		return F("image/jpeg");
	else if (filename.endsWith(F(".ico")))
		return F("image/x-icon");
	else if (filename.endsWith(F(".xml")))
		return F("text/xml");
	else if (filename.endsWith(F(".pdf")))
		return F("application/x-pdf");
	else if (filename.endsWith(F(".zip")))
		return F("application/x-zip");
	else if (filename.endsWith(F(".gz")))
		return F("application/x-gzip");
	return F("text/plain");
}

void WebServer::handleFileRead(AsyncWebServerRequest *request)
{
	auto path = request->url();
	LOG_DEBUG(F("handleFileRead: ") << path);

	const bool worksWithoutAuth = path.startsWith(F("/media/")) ||
								  path.startsWith(F("/js/")) ||
								  path.startsWith(F("/css/")) ||
								  path.startsWith(F("/font/"));

	if (!worksWithoutAuth && !isAuthenticated(request))
	{
		LOG_DEBUG(F("Redirecting to login page"));
		path = F("/login.html");
	}
	else
	{
		if (path.endsWith("/"))
		{
			path += F("/index.html"); // If a folder is requested, send the index file
		}
	}

	const auto contentType = getContentType(path); // Get the MIME type

	for (auto &&entry : staticFilesMap)
	{
		if (request->url().equalsIgnoreCase(entry.Path))
		{
			AsyncResponseStream *response = request->beginResponseStream(contentType, 1024);
			response->addHeader(F("Cache-Control"), F("public, max-age=31536000"));
			response->write(entry.Data, entry.Size);
			if (entry.Zipped)
			{
				response->addHeader(F("Content-Encoding"), F("gzip"));
			}
			request->send(response);
			LOG_DEBUG(F("Served from Memory file path:") << path);
			return;
		}
	}

	{
		path = F("/web") + path;
		const auto pathWithGz = path + F(".gz");
		if (LittleFS.exists(pathWithGz) || LittleFS.exists(path))
		{ // If the file exists, either as a compressed archive, or normal
			const bool gzipped = LittleFS.exists(pathWithGz);
			if (gzipped)
			{					  // If there's a compressed version available
				path += F(".gz"); // Use the compressed version
			}

			auto response = request->beginResponse(LittleFS, path, contentType);
			if (worksWithoutAuth)
			{
				response->addHeader(F("Cache-Control"), F("public, max-age=31536000"));
			}

			if (gzipped)
			{
				response->addHeader(F("Content-Encoding"), F("gzip"));
			}
			request->send(response);

			LOG_DEBUG(F("Served file path:") << path);
		}
		else
		{
			handleNotFound(request);
		}
	}
}

/** Redirect to captive portal if we got a request for another domain.
 * Return true in that case so the page handler do not try to handle the request again. */
bool WebServer::isCaptivePortalRequest(AsyncWebServerRequest *request)
{
	if (!isIp(request->host()))
	{
		LOG_INFO(F("Request redirected to captive portal"));
		AsyncWebServerResponse *response = request->beginResponse(302, F("text/plain"));
		response->addHeader(F("Location"), String(F("http://")) + toStringIp(request->client()->localIP()));
		request->send(response);
		return true;
	}
	return false;
}

void WebServer::handleNotFound(AsyncWebServerRequest *request)
{
	if (isCaptivePortalRequest(request))
	{
		// if captive portal redirect instead of displaying the error page
		return;
	}

	String message = F("File Not Found\n\n");
	message += F("URI: ");
	message += request->url();
	message += F("\nMethod: ");
	message += (request->method() == HTTP_GET) ? F("GET") : F("POST");
	message += F("\nArguments: ");
	message += request->args();
	message += F("\n");

	for (unsigned int i = 0; i < request->args(); i++)
	{
		message += " " + request->argName(i) + ": " + request->arg(i) + "\n";
	}

	LOG_INFO(message);

	AsyncWebServerResponse *response = request->beginResponse(404, F("text/plain"), message);
	response->addHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
	response->addHeader(F("(Pragma"), F("no-cache"));
	response->addHeader(F("Expires"), F("-1"));
	request->send(response);
}

// is this an IP?
bool WebServer::isIp(const String &str)
{
	for (unsigned int i = 0; i < str.length(); i++)
	{
		int c = str.charAt(i);
		if (c != '.' && (c < '0' || c > '9'))
		{
			return false;
		}
	}
	return true;
}

String WebServer::toStringIp(const IPAddress &ip)
{
	return ip.toString();
}

void WebServer::redirectToRoot(AsyncWebServerRequest *request)
{
	AsyncWebServerResponse *response = request->beginResponse(301); // Sends 301 redirect
	response->addHeader(F("Location"), F("/"));
	request->send(response);
}
