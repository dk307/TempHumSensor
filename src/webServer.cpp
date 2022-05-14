#include "webServer.h"

#include <hash.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <StreamString.h>

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
	const uint32_t Size;
	const char *MediaType;
	const uint8_t Zipped;
} StaticFilesMap;

static const char JsonMediaType[] PROGMEM = "application/json";
static const char JsMediaType[] PROGMEM = "text/javascript";
static const char HtmlMediaType[] PROGMEM = "text/html";
static const char CssMediaType[] PROGMEM = "text/css";
static const char PngMediaType[] PROGMEM = "image/png";
static const char TextPlainMediaType[] PROGMEM = "text/plain";

static const char LoginUrl[] PROGMEM = "/login.html";
static const char IndexUrl[] PROGMEM = "/index.html";
static const char LogoUrl[] PROGMEM = "/media/logo.png";
static const char FaviconUrl[] PROGMEM = "/media/favicon.png";
static const char LogoutUrl[] PROGMEM = "/media/logout.png";
static const char SettingsUrl[] PROGMEM = "/media/settings.png";
static const char AllJsUrl[] PROGMEM = "/js/s.js";
static const char MdbCssUrl[] PROGMEM = "/css/mdb.min.css";

static const char MD5Header[] PROGMEM = "md5";
static const char CacheControlHeader[] PROGMEM = "Cache-Control";
static const char CookieHeader[] PROGMEM = "Cookie";

const static StaticFilesMap staticFilesMap[] PROGMEM = {
	{IndexUrl, index_html_gz, index_html_gz_len, HtmlMediaType, true},
	{LoginUrl, login_html_gz, login_html_gz_len, HtmlMediaType, true},
	{LogoUrl, logo_png, logo_png_len, PngMediaType, false},
	{FaviconUrl, logo_png, logo_png_len, PngMediaType, false},
	{AllJsUrl, s_js_gz, s_js_gz_len, JsMediaType, true},
	{MdbCssUrl, mdb_min_css_gz, mdb_min_css_gz_len, CssMediaType, true},
};

WebServer WebServer::instance;

void WebServer::begin()
{
	LOG_DEBUG(F("WebServer Starting"));
	events.onConnect(std::bind(&WebServer::onEventConnect, this, std::placeholders::_1));
	logging.onConnect(std::bind(&WebServer::onLoggingConnect, this, std::placeholders::_1));
	events.setFilter(std::bind(&WebServer::filterEvents, this, std::placeholders::_1));
	logging.setFilter(std::bind(&WebServer::filterEvents, this, std::placeholders::_1));
	Logger.setMsgCallback(std::bind(&WebServer::sendLogs, this, std::placeholders::_1));
	httpServer.addHandler(&events);
	httpServer.addHandler(&logging);
	httpServer.begin();
	serverRouting();
	LOG_INFO(F("WebServer Started"));

	hardware::instance.temperatureChangeCallback.addConfigSaveCallback(std::bind(&WebServer::notifyTemperatureChange, this));
	hardware::instance.humidityChangeCallback.addConfigSaveCallback(std::bind(&WebServer::notifyHumidityChange, this));
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

bool WebServer::filterEvents(AsyncWebServerRequest *request)
{
	if (!isAuthenticated(request))
	{
		LOG_WARNING(F("Dropping events request"));
		return false;
	}
	return true;
}

void WebServer::serverRouting()
{
	// form calls
	httpServer.on(("/login.handler"), HTTP_POST, handleLogin);
	httpServer.on(("/logout.handler"), HTTP_POST, handleLogout);
	httpServer.on(("/wifiupdate.handler"), HTTP_POST, wifiUpdate);

	httpServer.on(("/othersettings.update.handler"), HTTP_POST, otherSettingsUpdate);
	httpServer.on(("/weblogin.update.handler"), HTTP_POST, webLoginUpdate);

	// ajax form call
	httpServer.on(("/factory.reset.handler"), HTTP_POST, factoryReset);
	httpServer.on(("/homekit.reset.handler"), HTTP_POST, homekitReset);
	httpServer.on(("/firmware.update.handler"), HTTP_POST, rebootOnUploadComplete, firmwareUpdateUpload);
	httpServer.on(("/setting.restore.handler"), HTTP_POST, rebootOnUploadComplete, restoreConfigurationUpload);
	httpServer.on(("/restart.handler"), HTTP_POST, restartDevice);

	// json ajax calls
	httpServer.on(("/api/sensor/get"), HTTP_GET, sensorGet);
	httpServer.on(("/api/wifi/get"), HTTP_GET, wifiGet);
	httpServer.on(("/api/information/get"), HTTP_GET, informationGet);
	httpServer.on(("/api/homekit/get"), HTTP_GET, homekitGet);
	httpServer.on(("/api/config/get"), HTTP_GET, configGet);

	httpServer.onNotFound(handleFileRead);
}

void WebServer::onEventConnect(AsyncEventSourceClient *client)
{
	if (client->lastId())
	{
		LOG_INFO(F("Events client reconnect"));
	}
	else
	{
		LOG_INFO(F("Events client first time"));
		// send all the events
		notifyTemperatureChange();
		notifyHumidityChange();
	}
}

void WebServer::onLoggingConnect(AsyncEventSourceClient *)
{
	Logger.enableLogging();
}

void WebServer::wifiGet(AsyncWebServerRequest *request)
{
	LOG_DEBUG(F("/api/wifi/get"));
	if (!manageSecurity(request))
	{
		return;
	}

	auto response = new AsyncJsonResponse(false, 256);
	auto jsonBuffer = response->getRoot();

	jsonBuffer[F("captivePortal")] = WifiManager::instance.isCaptivePortal();
	jsonBuffer[F("ssid")] = WifiManager::SSID();

	response->setLength();
	request->send(response);
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
		handleError(request, F("Required parameters not provided"), 400);
	}
}

template <class Array, class K, class T>
void WebServer::addKeyValueObject(Array &array, const K &key, const T &value)
{
	auto j1 = array.createNestedObject();
	j1[F("key")] = key;
	j1[F("value")] = value;
}

String WebServer::GetUptime()
{
	const auto now = millis() / 1000;
	const auto hour = now / 3600;
	const auto mins = (now % 3600) / 60;
	const auto sec = (now % 3600) % 60;

	StreamString upTime;
	upTime.reserve(30U);
	upTime.printf_P(PSTR("%02d hours %02d mins %02d secs"), hour, mins, sec);
	return upTime;
}

void WebServer::informationGet(AsyncWebServerRequest *request)
{
	LOG_DEBUG(F("/api/information/get"));
	if (!manageSecurity(request))
	{
		return;
	}

	const auto maxFreeHeapSize = ESP.getMaxFreeBlockSize() / 1024;
	const auto freeHeap = ESP.getFreeHeap() / 1024;

	auto response = new AsyncJsonResponse(true, 1024);
	auto arr = response->getRoot();

	addKeyValueObject(arr, F("Version"), VERSION);
	addKeyValueObject(arr, F("Uptime"), GetUptime());
	addKeyValueObject(arr, F("AP SSID"), WiFi.SSID());
	addKeyValueObject(arr, F("AP Signal Strength"), WiFi.RSSI());
	addKeyValueObject(arr, F("Mac Address"), WiFi.softAPmacAddress());

	addKeyValueObject(arr, F("Reset Reason"), ESP.getResetReason());
	addKeyValueObject(arr, F("CPU Frequency (MHz)"), system_get_cpu_freq());

	addKeyValueObject(arr, F("Max Block Free Size (KB)"), maxFreeHeapSize);
	addKeyValueObject(arr, F("Free Heap (KB)"), freeHeap);

	FSInfo fsInfo;
	LittleFS.info(fsInfo);

	addKeyValueObject(arr, F("Filesystem Total Size (KB)"), fsInfo.totalBytes / 1024);
	addKeyValueObject(arr, F("Filesystem Free Size (KB)"), (fsInfo.totalBytes - fsInfo.usedBytes) / 1024);

	response->setLength();
	request->send(response);
}

void WebServer::homekitGet(AsyncWebServerRequest *request)
{
	LOG_DEBUG(F("/api/homekit/get"));
	if (!manageSecurity(request))
	{
		return;
	}

	auto response = new AsyncJsonResponse(true, 1024);
	auto arr = response->getRoot();

	const bool paired = homeKit2::instance.isPaired();

	addKeyValueObject(arr, F("Paired"), homeKit2::instance.isPaired() ? F("Yes") : F("No"));
	if (!paired)
	{
		addKeyValueObject(arr, F("Password"), homeKit2::instance.getPassword());
	}
	else
	{
		addKeyValueObject(arr, F("Connected Clients Count"), homeKit2::instance.getConnectedClientsCount());
	}

	response->setLength();
	request->send(response);
}

void WebServer::configGet(AsyncWebServerRequest *request)
{
	LOG_DEBUG(F("/api/information/get"));
	if (!manageSecurity(request))
	{
		return;
	}
	const auto json = config::instance.getAllConfigAsJson();
	request->send(200, FPSTR(JsonMediaType), json);
}

template <class V, class T>
void WebServer::addToJsonDoc(V &doc, T id, float value)
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
	auto response = new AsyncJsonResponse(false, 256);
	auto doc = response->getRoot();

	addToJsonDoc(doc, F("humidity"), hardware::instance.getHumidity());
	addToJsonDoc(doc, F("temperatureC"), hardware::instance.getTemperatureC());

	response->setLength();
	request->send(response);
}

// Check if header is present and correct
bool WebServer::isAuthenticated(AsyncWebServerRequest *request)
{
	LOG_TRACE(F("Checking if authenticated"));
	if (request->hasHeader(FPSTR(CookieHeader)))
	{
		const String cookie = request->header(FPSTR(CookieHeader));
		LOG_TRACE(F("Found cookie: ") << cookie);

		const String token = sha1(config::instance.data.webUserName + ":" +
								  config::instance.data.webPassword + ":" +
								  request->client()->remoteIP().toString());

		if (cookie.indexOf(F("ESPSESSIONID=") + token) != -1)
		{
			LOG_TRACE(F("Authentication Successful"));
			return true;
		}
	}
	LOG_TRACE(F("Authentication Failed"));
	return false;
}

void WebServer::handleLogin(AsyncWebServerRequest *request)
{
	const auto UserNameParameter = F("username");
	const auto PasswordParameter = F("password");

	LOG_INFO(F("Handle login"));
	String msg;
	if (request->hasHeader(FPSTR(CookieHeader)))
	{
		// Print cookies
		LOG_TRACE(F("Found cookie: ") << request->header(FPSTR(CookieHeader)));
	}

	if (request->hasArg(UserNameParameter) && request->hasArg(PasswordParameter))
	{
		if (request->arg(UserNameParameter).equalsIgnoreCase(config::instance.data.webUserName) &&
			request->arg(PasswordParameter).equalsConstantTime(config::instance.data.webPassword))
		{
			LOG_INFO(F("User/Password correct"));
			auto response = request->beginResponse(301); // Sends 301 redirect

			response->addHeader(F("Location"), F("/"));
			response->addHeader(FPSTR(CacheControlHeader), F("no-cache"));

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
		response->addHeader(FPSTR(CacheControlHeader), F("no-cache"));
		request->send(response);
		return;
	}
	else
	{
		handleError(request, F("Login Parameter not provided"), 400);
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
	response->addHeader(FPSTR(CacheControlHeader), F("no-cache"));
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
		handleError(request, F("Correct Parameters not provided"), 400);
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
		config::instance.data.showDisplayInF = !request->arg(showDisplayInF).equalsIgnoreCase(F("on"));
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

	request->send(200);
	hardware::instance.showExternalMessages(F("Restarting"), String());
	operations::instance.reboot();
}

void WebServer::factoryReset(AsyncWebServerRequest *request)
{
	LOG_WARNING(F("factoryReset"));

	if (!manageSecurity(request))
	{
		return;
	}

	request->send(200);
	hardware::instance.showExternalMessages(F("Factory"), F("Reset"));
	operations::instance.factoryReset();
}

void WebServer::rebootOnUploadComplete(AsyncWebServerRequest *request)
{
	LOG_INFO(F("reboot"));

	if (!manageSecurity(request))
	{
		return;
	}

	request->send(200);
	hardware::instance.showExternalMessages(F("Rebooting"), String());
	operations::instance.reboot();
}

void WebServer::homekitReset(AsyncWebServerRequest *request)
{
	LOG_INFO(F("homekitReset"));

	if (!manageSecurity(request))
	{
		return;
	}

	config::instance.data.homeKitPairData.resize(0);
	config::instance.save();
	request->send(200);
	hardware::instance.showExternalMessages(F("Rebooting"), String());
	operations::instance.reboot();
}

void WebServer::handleFileRead(AsyncWebServerRequest *request)
{
	auto path = request->url();
	LOG_DEBUG(F("handleFileRead: ") << path);

	if (path.endsWith(F("/")) || path.isEmpty())
	{
		LOG_DEBUG(F("Redirecting to index page"));
		path = FPSTR(IndexUrl);
	}

	const bool worksWithoutAuth = path.startsWith(F("/media/")) ||
								  path.startsWith(F("/js/")) ||
								  path.startsWith(F("/css/")) ||
								  path.startsWith(F("/font/")) ||
								  path.equalsIgnoreCase(FPSTR(LoginUrl));

	if (!worksWithoutAuth && !isAuthenticated(request))
	{
		LOG_DEBUG(F("Redirecting to login page"));
		path = String(FPSTR(LoginUrl));
	}

	for (size_t i = 0; i < sizeof(staticFilesMap) / sizeof(staticFilesMap[0]); i++)
	{
		const auto entryPath = FPSTR(pgm_read_ptr(&staticFilesMap[i].Path));
		if (path.equalsIgnoreCase(entryPath))
		{
			const auto mediaType = FPSTR(pgm_read_ptr(&staticFilesMap[i].MediaType));
			const auto data = reinterpret_cast<const uint8_t *>(pgm_read_ptr(&staticFilesMap[i].Data));
			const auto size = pgm_read_dword(&staticFilesMap[i].Size);
			const auto zipped = pgm_read_byte(&staticFilesMap[i].Zipped);
			auto response = request->beginResponse_P(200, String(mediaType), data, size);
			if (worksWithoutAuth)
			{
				response->addHeader(FPSTR(CacheControlHeader), F("public, max-age=31536000"));
			}
			if (zipped)
			{
				response->addHeader(F("Content-Encoding"), F("gzip"));
			}
			request->send(response);
			LOG_DEBUG(F("Served path:") << path << F(" mimeType:") << mediaType
										<< F(" size:") << size);
			return;
		}
	}

	handleNotFound(request);
}

/** Redirect to captive portal if we got a request for another domain.
 * Return true in that case so the page handler do not try to handle the request again. */
bool WebServer::isCaptivePortalRequest(AsyncWebServerRequest *request)
{
	if (!isIp(request->host()))
	{
		LOG_INFO(F("Request redirected to captive portal"));
		AsyncWebServerResponse *response = request->beginResponse(302, TextPlainMediaType);
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
		message += F(" ") + request->argName(i) + F(": ") + request->arg(i) + "\n";
	}

	handleError(request, message, 404);
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

void WebServer::firmwareUpdateUpload(AsyncWebServerRequest *request,
									 const String &filename,
									 size_t index,
									 uint8_t *data,
									 size_t len,
									 bool final)
{
	LOG_DEBUG(F("firmwareUpdateUpload"));

	if (!manageSecurity(request))
	{
		return;
	}

	String error;
	if (!index)
	{
		String md5;

		if (request->hasHeader(FPSTR(MD5Header)))
		{
			md5 = request->getHeader(FPSTR(MD5Header))->value();
		}

		LOG_DEBUG(F("Expected MD5:") << md5);

		if (md5.length() != 32)
		{
			handleError(request, F("MD5 parameter invalid. Check file exists."), 500);
			return;
		}

		if (operations::instance.startUpdate(request->contentLength(), md5, error))
		{
			hardware::instance.showExternalMessages(F("Updating"), String());
			// success, let's make sure we end the update if the client hangs up
			request->onDisconnect(handleEarlyUpdateDisconnect);
		}
		else
		{
			handleError(request, error, 500);
			return;
		}
	}

	if (operations::instance.isUpdateInProgress())
	{
		if (!operations::instance.writeUpdate(data, len, error))
		{
			handleError(request, error, 500);
		}

		if (final)
		{
			if (!operations::instance.endUpdate(error))
			{
				handleError(request, error, 500);
			}
		}
	}
}

void WebServer::restoreConfigurationUpload(AsyncWebServerRequest *request,
										   const String &filename,
										   size_t index,
										   uint8_t *data,
										   size_t len,
										   bool final)
{
	LOG_DEBUG(F("restoreConfigurationUpload"));

	if (!manageSecurity(request))
	{
		return;
	}

	String error;
	if (!index)
	{
		WebServer::instance.restoreConfigData = std::make_unique<std::vector<uint8_t>>();
	}

	for (size_t i = 0; i < len; i++)
	{
		WebServer::instance.restoreConfigData->push_back(data[i]);
	}

	if (final)
	{
		String md5;
		if (request->hasHeader(FPSTR(MD5Header)))
		{
			md5 = request->getHeader(FPSTR(MD5Header))->value();
		}

		LOG_DEBUG(F("Expected MD5:") << md5);

		if (md5.length() != 32)
		{
			handleError(request, F("MD5 parameter invalid. Check file exists."), 500);
			return;
		}

		if (!config::instance.restoreAllConfigAsJson(*WebServer::instance.restoreConfigData, md5))
		{
			handleError(request, F("Restore Failed"), 500);
			return;
		}
	}
}

void WebServer::handleError(AsyncWebServerRequest *request, const String &message, int code)
{
	if (!message.isEmpty())
	{
		LOG_INFO(message);
	}
	AsyncWebServerResponse *response = request->beginResponse(code, TextPlainMediaType, message);
	response->addHeader(FPSTR(CacheControlHeader), F("no-cache, no-store, must-revalidate"));
	response->addHeader(F("Pragma"), F("no-cache"));
	response->addHeader(F("Expires"), F("-1"));
	request->send(response);
}

void WebServer::handleEarlyUpdateDisconnect()
{
	operations::instance.abortUpdate();
}

void WebServer::notifyTemperatureChange()
{
	if (events.count())
	{
		events.send(String(hardware::instance.getTemperatureC()).c_str(), "temperature", millis());
	}
}

void WebServer::notifyHumidityChange()
{
	if (events.count())
	{
		events.send(String(hardware::instance.getHumidity()).c_str(), "humidity", millis());
	}
}

bool WebServer::sendLogs(const String &data)
{
	// Serial.println(data);
	if (logging.count() > 0)
	{
		logging.send(data.c_str(), "logs", millis());
		return true;
	}

	return false;
}