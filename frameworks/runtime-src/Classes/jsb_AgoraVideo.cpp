//
//  jsb_agoraVideo.cpp
//
//  Created by becklee on 18/3/3
//
//

#include "jsb_AgoraVideo.h"

#include "cocos2d.h"
#include "spidermonkey_specifics.h"
#include "ScriptingCore.h"
#include "cocos2d_specifics.hpp"
#include <string.h>
#include "../../../AgoraGamingSDK/include/agora_cocos2dx.h"

using namespace cocos2d;
using namespace agora::rtc::cocos2dx;

class CagoraVideoJsWrapper;
CagoraVideoJsWrapper* g_SingleInstance;

JSClass  *js_cocos2dx_agoraVideo_class;
JSObject *js_cocos2dx_agoraVideo_prototype;

class CagoraVideoJsWrapper: public IAgoraCocos2dxEngineEventHandler {
public:
//    static CagoraVideoJsWrapper* getInstance();
    CagoraVideoJsWrapper();
    ~CagoraVideoJsWrapper();

public:
    mozilla::Maybe<JS::PersistentRootedObject> _JSDelegate;
    inline IAgoraCocos2dxEngine* agora() {
        return AgoraRtcEngineForGaming_getInstance();
    }
    
    Sprite* localVideoSprite() { return _localVideoSprite; }
    Sprite* remoteVideoSprite(uint32_t uid) {
        auto itr = _remoteVideoSprites.find(uid);
        return itr != _remoteVideoSprites.end() ? itr->second : nullptr;
    }
    int joinChannel(const std::string& channelName, uint32_t uid, int videoEnabled, int videoMode, const std::string& info);
    int leaveChannel();

private:
    void clear();
    
    Sprite* _localVideoSprite;
    std::map<uint32_t, Sprite*> _remoteVideoSprites;
    
//    static CagoraVideoJsWrapper* _instance;
    
private:
    /* IAgoraCocos2dxEngineEventHandler callbacks */
    virtual void onJoinChannelSuccess(const char* channel, uid_t uid, int elapsed);
    virtual void onLeaveChannel(const RtcStats& stats);
    virtual void onUserJoined(uid_t uid, int elapsed);
    virtual void onUserOffline(uid_t uid, USER_OFFLINE_REASON_TYPE reason);
    virtual void onAudioRouteChanged(AUDIO_ROUTE_TYPE routing);
    virtual void onRequestChannelKey();
    virtual void onUserMuteVideo(uint32_t uid, bool muted);
    virtual void onUserEnableVideo(uint32_t  uid, bool enabled);
    virtual void onRemoteVideoStats(RemoteVideoStats& stats);
    virtual void onLocalVideoStats(LocalVideoStats& stats);
    virtual void onFirstRemoteVideoFrame(uint32_t uid, int width, int height, int elapsed);
    virtual void onFirstLocalVideoFrame(uint32_t width, int height, int elapsed);
    virtual void onFirstRemoteVideoDecoded(uint32_t uid, int width, int height, int elapsed);
    virtual void onVideoSizeChanged(uint32_t uid, int width, int height, int rotation);
    virtual void onCameraReady();
    virtual void onVideoStopped();
};

void CagoraVideoJsWrapper::clear() {
    CC_SAFE_RELEASE_NULL(_localVideoSprite);
    for (auto& itr : _remoteVideoSprites) {
        CC_SAFE_RELEASE(itr.second);
    }
    _remoteVideoSprites.clear();
}

CagoraVideoJsWrapper::CagoraVideoJsWrapper(): _localVideoSprite(nullptr)
{
    JSContext* cx = ScriptingCore::getInstance()->getGlobalContext();
    _JSDelegate.construct(cx);
    agora()->setEventHandler(this);
}



CagoraVideoJsWrapper::~CagoraVideoJsWrapper()
{
    clear();
}

//CagoraVideoJsWrapper* CagoraVideoJsWrapper::getInstance()
//{
//    if (NULL == _instance) {
//        _instance = new CagoraVideoJsWrapper();
//    }
//    return _instance;
//}
#define AGORA_CHECK(error) { int code = error; if (code != 0) CCLOG("[Agora Error] %s", agora()->getErrorDescription(code)); }

int CagoraVideoJsWrapper::joinChannel(const std::string& channelName, uint32_t uid, int videoEnabled, int videoMode, const std::string& info)
{
    bool isVideoEnabled = (videoEnabled ? true : false);
    AGORA_CHECK(agora()->setChannelProfile(CHANNEL_PROFILE_LIVE_BROADCASTING));
    AGORA_CHECK(agora()->setClientRole(CLIENT_ROLE_BROADCASTER));
    if(isVideoEnabled){
        AGORA_CHECK(agora()->enableVideo());
    }
    AGORA_CHECK(agora()->setVideoProfile((VIDEO_PROFILE)videoMode, false));
    
    int error = agora()->joinChannel(channelName.c_str(), info.empty() ? "xxx" : info.c_str(), uid);
    return error;
}


int CagoraVideoJsWrapper::leaveChannel()
{
    AGORA_CHECK(agora()->stopPreview());
    int error = agora()->leaveChannel();
    
    if (_localVideoSprite != nullptr) {
        agora()->setupLocalVideoSprite(NULL);
        CC_SAFE_RELEASE_NULL(_localVideoSprite);
    }
    for (auto& itr : _remoteVideoSprites) {
        agora()->setupRemoteVideoSprite(NULL, itr.first);
        CC_SAFE_RELEASE(itr.second);
    }
    _remoteVideoSprites.clear();
    return error;
}

static void js_cocos2dx_agoraVideo_finalize(JSFreeOp *fop, JSObject *obj){
    CCLOG("jsbindings: finalizing JS object %p (AgoraVideo)", obj);
}

bool js_cocos2dx_extension_agoraVideo_constructor(JSContext *cx, uint32_t argc, jsval *vp)
{
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    if (argc == 0)
    {
        if (g_SingleInstance == NULL)
        {
            g_SingleInstance = new CagoraVideoJsWrapper();
        }
        
        JS::RootedObject obj(cx, JS_NewObject(cx, js_cocos2dx_agoraVideo_class, JS::RootedObject(cx, js_cocos2dx_agoraVideo_prototype), JS::NullPtr()));
        
        // link the native object with the javascript object

        g_SingleInstance->_JSDelegate.ref() = obj;
        js_proxy_t *p = jsb_new_proxy(g_SingleInstance, obj);
        JS::AddNamedObjectRoot(cx, &p->obj, "AgoraVideo");
        args.rval().set(OBJECT_TO_JSVAL(obj));
        
        return true;
    }
    
    JS_ReportError(cx, "wrong number of arguments: %d, was expecting %d", argc, 0);
    return false;
}

bool js_cocos2dx_extension_joinChannel(JSContext *cx, uint32_t argc, jsval *vp) {
    if (argc == 5)
    {
        JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
        
        std::string channelName;
        jsval_to_std_string(cx, args.get(0), &channelName);
        
        uint32_t uid;
        jsval_to_uint32(cx, args.get(1), &uid);
        
        int videoEnabled;
        jsval_to_int32(cx, args.get(2), &videoEnabled);
        
        int videoMode;
        jsval_to_int32(cx, args.get(3), &videoMode);
        
        std::string info;
        jsval_to_std_string(cx, args.get(0), &info);
        
        
        int error = g_SingleInstance->joinChannel(channelName, uid, videoEnabled, videoMode, info);
        
        args.rval().set(int32_to_jsval(cx, error));
        
        return true;
    }
    JS_ReportError(cx, "wrong number of arguments: %d, was expecting %d", argc, 5);
    return false;
}

bool js_cocos2dx_extension_leaveChannel(JSContext *cx, uint32_t argc, jsval *vp) {
    CCLOG("leaveChannel() Here !!!");
    if (argc == 0)
    {
        JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
        
        int iErrorcode = g_SingleInstance->leaveChannel();
        args.rval().set(int32_to_jsval(cx, iErrorcode));
        
        return true;
    }
    JS_ReportError(cx, "wrong number of arguments: %d, was expecting %d", argc, 0);
    return false;
}

bool js_cocos2dx_extension_getLocalSprite(JSContext *cx, uint32_t argc, jsval *vp) {
    CCLOG("getLocalSprite()  !!!");
    if (argc == 0)
    {
        JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
        
        auto sprite = g_SingleInstance->localVideoSprite();
        
        js_type_class_t *typeClass = js_get_type_from_native<Sprite>(sprite);
        
        JS::RootedObject jsret(cx, jsb_ref_autoreleased_create_jsobject(cx, sprite, typeClass, "cocos2d::Sprite"));
        args.rval().set(OBJECT_TO_JSVAL(jsret));
        
        return true;
    }
    
    JS_ReportError(cx, "wrong number of arguments: %d, was expecting %d", argc, 0);
    return false;
}

bool js_cocos2dx_extension_getRemoteSprites(JSContext *cx, uint32_t argc, jsval *vp) {
    CCLOG("getRemoteSprites()  !!!");
    if (argc == 1)
    {
        JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
        
        uint32_t uid;
        jsval_to_uint32(cx, args.get(0), &uid);
        
        auto sprite = g_SingleInstance->remoteVideoSprite(uid);
        
        js_type_class_t *typeClass = js_get_type_from_native<Sprite>(sprite);
        
        JS::RootedObject jsret(cx, jsb_ref_autoreleased_create_jsobject(cx, sprite, typeClass, "cocos2d::Sprite"));
        args.rval().set(OBJECT_TO_JSVAL(jsret));
        
        return true;
    }
    
    JS_ReportError(cx, "wrong number of arguments: %d, was expecting %d", argc, 0);
    return false;
}

bool js_cocos2dx_extension_getVersion(JSContext *cx, uint32_t argc, jsval *vp) {
    CCLOG("getVersion()  !!!");
    if (argc == 0)
    {
        JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
        
        int build = 0;
        const char* buildver = g_SingleInstance->agora()->getVersion(&build);
        
        JSContext* cx = ScriptingCore::getInstance()->getGlobalContext();
        args.rval().set(c_string_to_jsval(cx, buildver));
        
        return true;
    }
    
    JS_ReportError(cx, "wrong number of arguments: %d, was expecting %d", argc, 0);
    return false;
}


bool js_cocos2dx_extension_getErrorDescription(JSContext *cx, uint32_t argc, jsval *vp) {
    CCLOG("getErrorDescription() !!!");
    if (argc == 1)
    {
        JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
        int code = 0;
        jsval_to_int(cx, args.get(0), &code);
        
        const char* description = g_SingleInstance->agora()->getErrorDescription(code);
        
        JSContext* cx = ScriptingCore::getInstance()->getGlobalContext();
        args.rval().set(c_string_to_jsval(cx, description));
        
        return true;
    }
    
    JS_ReportError(cx, "wrong number of arguments: %d, was expecting %d", argc, 0);
    return false;
}


bool js_cocos2dx_extension_setLogFilter(JSContext *cx, uint32_t argc, jsval *vp) {
    CCLOG("setLogFilter() !!!");
    if (argc == 1)
    {
        JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
        int filter = 0;
        jsval_to_int(cx, args.get(0), &filter);
        
        int ret = g_SingleInstance->agora()->setLogFilter(filter);
   
        JSContext* cx = ScriptingCore::getInstance()->getGlobalContext();
        args.rval().set(int32_to_jsval(cx, ret));
        
        return true;
    }
    
    JS_ReportError(cx, "wrong number of arguments: %d, was expecting %d", argc, 0);
    return false;
    
}

bool js_cocos2dx_extension_setLogFile(JSContext *cx, uint32_t argc, jsval *vp) {
    CCLOG("setLogFile() !!!");
    if (argc == 1)
    {
        JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
        std::string filePath;
        jsval_to_std_string(cx, args.get(0), &filePath);
//        CCLOG("filePath : %s !", filePath.c_str());
        int ret = g_SingleInstance->agora()->setLogFile(filePath.c_str());
        
        JSContext* cx = ScriptingCore::getInstance()->getGlobalContext();
        args.rval().set(int32_to_jsval(cx, ret));
        
        return true;
    }
    
    JS_ReportError(cx, "wrong number of arguments: %d, was expecting %d", argc, 0);
    return false;
}

bool js_cocos2dx_extension_setParameters(JSContext *cx, uint32_t argc, jsval *vp) {
    CCLOG("setParameters() !!!");
    if (argc == 1)
    {
        JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
        std::string strVal;
        jsval_to_std_string(cx, args.get(0), &strVal);
        int ret = g_SingleInstance->agora()->setParameters(strVal.c_str());
        
        JSContext* cx = ScriptingCore::getInstance()->getGlobalContext();
        args.rval().set(int32_to_jsval(cx, ret));
        
        return true;
    }
    
    JS_ReportError(cx, "wrong number of arguments: %d, was expecting %d", argc, 0);
    return false;
    
}

bool js_cocos2dx_extension_enableAudio(JSContext *cx, uint32_t argc, jsval *vp) {
    CCLOG("enableAudio() !!!");
    if (argc == 0)
    {
        JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
        std::string strVal;
        int  ret = g_SingleInstance->agora()->enableAudio();
        JSContext* cx = ScriptingCore::getInstance()->getGlobalContext();
        args.rval().set(int32_to_jsval(cx, ret));
        
        return true;
    }
    
    JS_ReportError(cx, "wrong number of arguments: %d, was expecting %d", argc, 0);
    return false;
}

bool js_cocos2dx_extension_disableAudio(JSContext *cx, uint32_t argc, jsval *vp) {
    CCLOG("disableAudio() !!!");
    if (argc == 0)
    {
        JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
        int ret = g_SingleInstance->agora()->disableAudio();
        
        JSContext* cx = ScriptingCore::getInstance()->getGlobalContext();
        args.rval().set(int32_to_jsval(cx, ret));
                
        return true;
    }
            
    JS_ReportError(cx, "wrong number of arguments: %d, was expecting %d", argc, 0);
    return false;
}
bool js_cocos2dx_extension_enableVideo(JSContext *cx, uint32_t argc, jsval *vp) {
    CCLOG("enableVideo() !!!");
    if (argc == 0)
    {
        JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
        int ret = g_SingleInstance->agora()->enableVideo();
        
        JSContext* cx = ScriptingCore::getInstance()->getGlobalContext();
        args.rval().set(int32_to_jsval(cx, ret));
        
        return true;
    }
    
    JS_ReportError(cx, "wrong number of arguments: %d, was expecting %d", argc, 0);
    return false;
}


bool js_cocos2dx_extension_disableVideo(JSContext *cx, uint32_t argc, jsval *vp) {
    CCLOG("disableVideo() !!!");
    if (argc == 0)
    {
        JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
        int ret = g_SingleInstance->agora()->disableVideo();
        
        JSContext* cx = ScriptingCore::getInstance()->getGlobalContext();
        args.rval().set(int32_to_jsval(cx, ret));
        
        return true;
    }
    
    JS_ReportError(cx, "wrong number of arguments: %d, was expecting %d", argc, 0);
    return false;
}


bool js_cocos2dx_extension_pause(JSContext *cx, uint32_t argc, jsval *vp) {
    CCLOG("pause() !!!");
    if (argc == 0)
    {
        JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
        g_SingleInstance->agora()->pause();
        
        return true;
    }
    
    JS_ReportError(cx, "wrong number of arguments: %d, was expecting %d", argc, 0);
    return false;
}

bool js_cocos2dx_extension_resume(JSContext *cx, uint32_t argc, jsval *vp) {
    CCLOG("resume() !!!");
    if (argc == 0)
    {
        g_SingleInstance->agora()->resume();

        return true;
    }
    
    JS_ReportError(cx, "wrong number of arguments: %d, was expecting %d", argc, 0);
    return false;
}

bool js_cocos2dx_extension_switchCamera(JSContext *cx, uint32_t argc, jsval *vp) {
    CCLOG("switchCamera() !!!");
    if (argc == 0)
    {
        JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
        int ret = g_SingleInstance->agora()->switchCamera();
    
        JSContext* cx = ScriptingCore::getInstance()->getGlobalContext();
        args.rval().set(int32_to_jsval(cx, ret));
        
        return true;
    }
    
    JS_ReportError(cx, "wrong number of arguments: %d, was expecting %d", argc, 0);
    return false;
}

bool js_cocos2dx_extension_appExit(JSContext *cx, uint32_t argc, jsval *vp) {
    CCLOG("appExit() !!!");
    if (argc == 0)
    {
        Director::getInstance()->end();
          CCLOG("appExit() 2 !!!");
        #if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
//            exit(0);
        #endif
        
        return true;
    }
    
    JS_ReportError(cx, "wrong number of arguments: %d, was expecting %d", argc, 0);
    return false;
}


void CagoraVideoJsWrapper::onJoinChannelSuccess(const char* channel, uid_t uid, int elapsed)
{

    CCLOG("[Agora]:onJoinChannelSuccess %s, %u, %d", channel, uid, elapsed);
    static char channelName[0x100];
    strcpy(channelName, channel);

    Director::getInstance()->getScheduler()->performFunctionInCocosThread([=] {
        if (_localVideoSprite == nullptr) {
            auto sprite = agora()->createSprite();
            sprite->setContentSize(Size(320, 180));
            agora()->setupLocalVideoSprite(sprite);
            sprite->retain();
            _localVideoSprite = sprite;
        } else {
            agora()->setupLocalVideoSprite(_localVideoSprite);
        }
        
        JSContext* cx = ScriptingCore::getInstance()->getGlobalContext();
        
        jsval params[3];
        params[0] = c_string_to_jsval(cx, channelName, -1);
        params[1] = uint32_to_jsval(cx, uid);
        params[2] = int32_to_jsval(cx, elapsed);
        
        ScriptingCore::getInstance()->executeFunctionWithOwner(OBJECT_TO_JSVAL(g_SingleInstance->_JSDelegate.ref()), "onJoinChannelSuccess", 3, params);
        
    });
    return ;

}

void CagoraVideoJsWrapper:: onLeaveChannel(const RtcStats& stats)
{
    CCLOG("[Agora]:onLeaveChannel %d, %d, %d", stats.totalDuration, stats.txBytes, stats.rxBytes);
    clear();
    
    Director::getInstance()->getScheduler()->performFunctionInCocosThread([=] {
        JSContext* cx = ScriptingCore::getInstance()->getGlobalContext();
        
        jsval params[12];
        params[0] = int32_to_jsval(cx, stats.totalDuration);
        params[1] = int32_to_jsval(cx, stats.txBytes);
        params[2] = int32_to_jsval(cx, stats.rxBytes);
        params[3] = int32_to_jsval(cx, stats.txKBitRate);
        params[4] = int32_to_jsval(cx, stats.rxKBitRate);
        params[5] = int32_to_jsval(cx, stats.txAudioKBitRate);
        params[6] = int32_to_jsval(cx, stats.rxAudioKBitRate);
        params[7] = int32_to_jsval(cx, stats.txVideoKBitRate);
        params[8] = int32_to_jsval(cx, stats.rxVideoKBitRate);
        params[9] = int32_to_jsval(cx, stats.users);
        params[10] = DOUBLE_TO_JSVAL(stats.cpuTotalUsage);
        params[11] = DOUBLE_TO_JSVAL(stats.cpuAppUsage);
        
        ScriptingCore::getInstance()->executeFunctionWithOwner(OBJECT_TO_JSVAL(g_SingleInstance->_JSDelegate.ref()), "onLeaveChannel", 12, params);
    });
    return ;
}

void CagoraVideoJsWrapper:: onUserJoined(uid_t uid, int elapsed)
{
     CCLOG("[Agora]:onUserJoined uid: %u", uid);
    
}
void CagoraVideoJsWrapper::onUserOffline(uid_t uid, USER_OFFLINE_REASON_TYPE reason)
{
    onUserEnableVideo(uid, false);
    CCLOG("[Agora]:onUserOffline uid: %u", uid);
}
void CagoraVideoJsWrapper::onAudioRouteChanged(AUDIO_ROUTE_TYPE routing)
{
     CCLOG("[Agora]:onAudioRouteChanged %d", routing);
    
}
void CagoraVideoJsWrapper:: onRequestChannelKey()
{
     CCLOG("[Agora]:onRequestChannelKey");
}
void CagoraVideoJsWrapper::onUserMuteVideo(uint32_t uid, bool muted)
{
     CCLOG("[Agora]:onUserMuteVideo uid:%u mute:%d", uid, muted);
}
void CagoraVideoJsWrapper:: onUserEnableVideo(uint32_t  uid, bool enabled)
{
    CCLOG("[Agora]:onUserEnableVideo uid:%u enable:%d", uid, enabled);
    Director::getInstance()->getScheduler()->performFunctionInCocosThread([=] {
        auto itr = _remoteVideoSprites.find(uid);
        if (itr != _remoteVideoSprites.end()) {
            if (!enabled) {
                agora()->setupRemoteVideoSprite(NULL, uid);
                CC_SAFE_RELEASE(itr->second);
                _remoteVideoSprites.erase(itr);
            }
        }
        JSContext* cx = ScriptingCore::getInstance()->getGlobalContext();
        
        jsval params[2];
        params[0] = uint32_to_jsval(cx, uid);
        if(enabled){
            params[1] = int32_to_jsval(cx, 1);
        }else{
            params[1] = int32_to_jsval(cx, 0);
        }
       
        ScriptingCore::getInstance()->executeFunctionWithOwner(OBJECT_TO_JSVAL(g_SingleInstance->_JSDelegate.ref()), "onUserEnableVideo", 2, params);
        
    });
}
                                                                          
void CagoraVideoJsWrapper:: onRemoteVideoStats(RemoteVideoStats& stats){
    CCLOG("[Agora]:onRemoteVideoStats uid:%u bitrate:%d framerate:%d", stats.uid, stats.receivedBitrate, stats.receivedFrameRate);
}

void CagoraVideoJsWrapper:: onLocalVideoStats(LocalVideoStats& stats){
     CCLOG("[Agora]:onLocalVideoStats bitrate:%d framerate:%d", stats.sentBitrate, stats.sentFrameRate);
}
void CagoraVideoJsWrapper:: onFirstRemoteVideoFrame(uint32_t uid, int width, int height, int elapsed){
    CCLOG("[Agora]:onFirstRemoteVideoFrame uid:%d width:%d height:%d", uid, width, height);
}
void CagoraVideoJsWrapper:: onFirstLocalVideoFrame(uint32_t width, int height, int elapsed){
     CCLOG("[Agora]:onFirstLocalVideoFram width:%d height:%d", width, height);
}
void CagoraVideoJsWrapper:: onFirstRemoteVideoDecoded(uint32_t uid, int width, int height, int elapsed){
    CCLOG("[Agora]:onFirstRemoteVideoDecoded uid:%u, width:%d height:%d", uid, width, height);
    if (1){
        Director::getInstance()->getScheduler()->performFunctionInCocosThread([=] {
            auto sprite = remoteVideoSprite(uid);
            if (sprite == nullptr) {
                sprite = agora()->createSprite();
                sprite->setContentSize(Size(320, 180));
                sprite->retain();
                _remoteVideoSprites[uid] = sprite;
            }
            agora()->setupRemoteVideoSprite(sprite, uid);
            
            JSContext* cx = ScriptingCore::getInstance()->getGlobalContext();
            jsval params[4];
            params[0] = uint32_to_jsval(cx, uid);
            params[1] = int32_to_jsval(cx, width);
            params[2] = int32_to_jsval(cx, height);
            params[3] = int32_to_jsval(cx, elapsed);
            
            ScriptingCore::getInstance()->executeFunctionWithOwner(OBJECT_TO_JSVAL(g_SingleInstance->_JSDelegate.ref()), "onFirstRemoteVideoDecoded", 4, params);
        });
    }
    
}
void CagoraVideoJsWrapper:: onVideoSizeChanged(uint32_t uid, int width, int height, int rotation){
     CCLOG("[Agora]:onVideoSizeChanged uid:%u, width:%d height:%d", uid, width, height);
}
void CagoraVideoJsWrapper::onCameraReady(){
    CCLOG("[Agora]:onCameraReady ");
}
void CagoraVideoJsWrapper:: onVideoStopped(){
    CCLOG("[Agora]:onVideoStopped");
}


void register_jsb_agoraVideo(JSContext *cx, JS::HandleObject global) {
    js_cocos2dx_agoraVideo_class = (JSClass *)calloc(1, sizeof(JSClass));
    js_cocos2dx_agoraVideo_class->name = "AgoraVideo";
    js_cocos2dx_agoraVideo_class->addProperty = JS_PropertyStub;
    js_cocos2dx_agoraVideo_class->delProperty = JS_DeletePropertyStub;
    js_cocos2dx_agoraVideo_class->getProperty = JS_PropertyStub;
    js_cocos2dx_agoraVideo_class->setProperty = JS_StrictPropertyStub;
    js_cocos2dx_agoraVideo_class->enumerate = JS_EnumerateStub;
    js_cocos2dx_agoraVideo_class->resolve = JS_ResolveStub;
    js_cocos2dx_agoraVideo_class->convert = JS_ConvertStub;
    js_cocos2dx_agoraVideo_class->finalize = js_cocos2dx_agoraVideo_finalize;
    js_cocos2dx_agoraVideo_class->flags = JSCLASS_HAS_RESERVED_SLOTS(2);
    
    static JSFunctionSpec funcs[] = {
     
        JS_FN("joinChannel", js_cocos2dx_extension_joinChannel, 5, JSPROP_PERMANENT | JSPROP_ENUMERATE),
        JS_FN("leaveChannel", js_cocos2dx_extension_leaveChannel, 0, JSPROP_PERMANENT | JSPROP_ENUMERATE),
        JS_FN("getLocalSprite", js_cocos2dx_extension_getLocalSprite, 0, JSPROP_PERMANENT | JSPROP_ENUMERATE),
        JS_FN("getRemoteSprites", js_cocos2dx_extension_getRemoteSprites, 0, JSPROP_PERMANENT | JSPROP_ENUMERATE),
        JS_FN("getVersion", js_cocos2dx_extension_getVersion, 0, JSPROP_PERMANENT | JSPROP_ENUMERATE),
        
        JS_FN("getErrorDescription", js_cocos2dx_extension_getErrorDescription, 0, JSPROP_PERMANENT | JSPROP_ENUMERATE),
        JS_FN("setLogFilter", js_cocos2dx_extension_setLogFilter, 0, JSPROP_PERMANENT | JSPROP_ENUMERATE),
        JS_FN("setLogFile", js_cocos2dx_extension_setLogFile, 0, JSPROP_PERMANENT | JSPROP_ENUMERATE),
        JS_FN("setParameters", js_cocos2dx_extension_setParameters, 0, JSPROP_PERMANENT | JSPROP_ENUMERATE),
        
        JS_FN("enableAudio", js_cocos2dx_extension_enableAudio, 0, JSPROP_PERMANENT | JSPROP_ENUMERATE),
        JS_FN("disableAudio", js_cocos2dx_extension_disableAudio, 0, JSPROP_PERMANENT | JSPROP_ENUMERATE),
        JS_FN("enableVideo", js_cocos2dx_extension_enableVideo, 0, JSPROP_PERMANENT | JSPROP_ENUMERATE),
        JS_FN("enableVideo", js_cocos2dx_extension_disableVideo, 0, JSPROP_PERMANENT | JSPROP_ENUMERATE),

        JS_FN("pause", js_cocos2dx_extension_pause, 0, JSPROP_PERMANENT | JSPROP_ENUMERATE),
        JS_FN("resume", js_cocos2dx_extension_resume, 0, JSPROP_PERMANENT | JSPROP_ENUMERATE),

        JS_FN("switchCamera", js_cocos2dx_extension_switchCamera, 0, JSPROP_PERMANENT | JSPROP_ENUMERATE),
        
//        JS_FN("muteLocalAudio", js_cocos2dx_extension_muteLocalAudio, 0, JSPROP_PERMANENT | JSPROP_ENUMERATE),
        
        JS_FN("appExit", js_cocos2dx_extension_appExit, 0, JSPROP_PERMANENT | JSPROP_ENUMERATE),
        
        JS_FS_END
    };
    
    static JSFunctionSpec st_funcs[] = {
        JS_FS_END
    };
    js_cocos2dx_agoraVideo_prototype = JS_InitClass(
                                                   cx, global,
                                                   JS::NullPtr(),
                                                   js_cocos2dx_agoraVideo_class,
                                                   js_cocos2dx_extension_agoraVideo_constructor, 0, // constructor
                                                   NULL,
                                                   funcs,
                                                   NULL, // no static properties
                                                   st_funcs);
    anonEvaluate(cx, global, "(function () { return AgoraVideo; })()").toObjectOrNull();
}
