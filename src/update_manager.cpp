#include "update_manager.h"
#include <WebServer.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>

extern WebServer server;

static const char* UPDATE_API="https://api.github.com/repos/walterz930/EmlaLock-esp32/releases/latest";
static String latestVersion;
static String latestUrl;
static bool managerReady=false;

const char* firmwareVersion(){return EMLALOCK_VERSION;}

static String normalizeVersion(String v){
  v.trim();
  if(v.startsWith("v")||v.startsWith("V"))v.remove(0,1);
  return v;
}

static int compareVersions(const String& left,const String& right){
  String a=normalizeVersion(left),b=normalizeVersion(right);
  int av[4]={0,0,0,0},bv[4]={0,0,0,0};
  String part="";int p=0;
  for(size_t i=0;i<=a.length()&&p<4;i++){
    if(i==a.length()||a[i]=='.'){av[p++]=part.toInt();part="";}
    else if(isDigit(a[i]))part+=a[i];
    else return 0;
  }
  part="";p=0;
  for(size_t i=0;i<=b.length()&&p<4;i++){
    if(i==b.length()||b[i]=='.'){bv[p++]=part.toInt();part="";}
    else if(isDigit(b[i]))part+=b[i];
    else return 0;
  }
  for(int i=0;i<4;i++){
    if(av[i]>bv[i])return 1;
    if(av[i]<bv[i])return -1;
  }
  return 0;
}

static bool newerVersion(const String& latest,const String& current){return compareVersions(latest,current)>0;}

static String targetAssetName(){
  String model=ESP.getChipModel();
  model.toUpperCase();
  if(model.indexOf("C3")>=0)return "EmlaLock-"+latestVersion+"-esp32c3.bin";
  if(model.indexOf("C6")>=0)return "EmlaLock-"+latestVersion+"-esp32c6.bin";
  if(model.indexOf("S2")>=0)return "EmlaLock-"+latestVersion+"-esp32s2.bin";
  if(model.indexOf("S3")>=0)return "EmlaLock-"+latestVersion+"-esp32s3.bin";
  return "EmlaLock-"+latestVersion+"-esp32.bin";
}

static bool fetchLatest(){
  latestVersion="";latestUrl="";
  WiFiClientSecure client;client.setInsecure();
  HTTPClient http;
  String url=String(UPDATE_API)+"?ts="+String(millis());
  if(!http.begin(client,url))return false;
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  http.setRedirectLimit(5);
  http.addHeader("Accept","application/vnd.github+json");
  http.addHeader("User-Agent","EmlaLock-ESP32");
  http.addHeader("Cache-Control","no-cache");
  int code=http.GET();
  if(code!=200){
    Serial.printf("OTA release check HTTP %d\n",code);
    http.end();
    return false;
  }
  String body=http.getString();http.end();
  JsonDocument doc;
  if(deserializeJson(doc,body))return false;
  latestVersion=normalizeVersion(doc["tag_name"].as<String>());
  String wanted=targetAssetName();
  String fallback;
  JsonArray assets=doc["assets"].as<JsonArray>();
  for(JsonObject asset:assets){
    String name=asset["name"].as<String>();
    String url=asset["browser_download_url"].as<String>();
    if(name==wanted){latestUrl=url;break;}
    if(name.endsWith("-esp32.bin"))fallback=url;
  }
  if(latestUrl.length()==0)latestUrl=fallback;
  return latestVersion.length()>0&&latestUrl.length()>0;
}

static void sendUpdateJson(int code,JsonDocument&d){String out;serializeJson(d,out);server.send(code,"application/json",out);}

void handleUpdateCheck(){
  JsonDocument d;
  d["current"]=firmwareVersion();
  if(WiFi.status()!=WL_CONNECTED){d["ok"]=false;d["error"]="Wi-Fi is not connected";sendUpdateJson(200,d);return;}
  if(!fetchLatest()){d["ok"]=false;d["error"]="Could not check GitHub for updates";sendUpdateJson(200,d);return;}
  d["ok"]=true;
  d["latest"]=latestVersion;
  d["update"]=newerVersion(latestVersion,firmwareVersion());
  sendUpdateJson(200,d);
}

void handleUpdateStart(){
  if(WiFi.status()!=WL_CONNECTED){server.send(503,"application/json",R"({"ok":false,"error":"Wi-Fi is not connected"})");return;}
  if(!fetchLatest()||!newerVersion(latestVersion,firmwareVersion())){server.send(200,"application/json",R"({"ok":true,"update":false})");return;}

  Serial.printf("Starting OTA %s -> %s (%s)\n",firmwareVersion(),latestVersion.c_str(),ESP.getChipModel());
  server.send(200,"application/json",R"({"ok":true,"updating":true})");
  delay(250);

  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(30);
  HTTPClient http;
  if(!http.begin(client,latestUrl)){
    Serial.println("OTA download could not be started");
    return;
  }
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  http.setRedirectLimit(5);
  http.addHeader("User-Agent","EmlaLock-ESP32");
  http.addHeader("Accept","application/octet-stream");

  int code=http.GET();
  if(code!=HTTP_CODE_OK){
    Serial.printf("OTA download HTTP %d\n",code);
    http.end();
    return;
  }

  int total=http.getSize();
  if(total>0)Serial.printf("OTA download started: %d bytes\n",total);
  else Serial.println("OTA download started with unknown content length");

  if(!Update.begin(total>0?(size_t)total:UPDATE_SIZE_UNKNOWN)){
    Update.printError(Serial);
    http.end();
    return;
  }

  WiFiClient* stream=http.getStreamPtr();
  size_t written=Update.writeStream(*stream);
  bool finished=(total<=0)||(written==(size_t)total);
  if(!finished)Serial.printf("OTA download incomplete: %u/%d bytes\n",(unsigned)written,total);

  if(finished&&Update.end(true)){
    Serial.printf("OTA update complete: %u bytes. Restarting...\n",(unsigned)written);
    http.end();
    delay(500);
    ESP.restart();
    return;
  }

  Update.printError(Serial);
  http.end();
}

void initUpdateManager(){
  if(managerReady)return;
  managerReady=true;
  server.on("/api/update/check",HTTP_GET,handleUpdateCheck);
  server.on("/api/update/start",HTTP_POST,handleUpdateStart);
}
