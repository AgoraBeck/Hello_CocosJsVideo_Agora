LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := agora-cocos2dx
LOCAL_SRC_FILES := ../../../../../AgoraGamingSDK/libs/android/$(TARGET_ARCH_ABI)/libagora-cocos2dx.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := agora-rtc
LOCAL_SRC_FILES := ../../../../../AgoraGamingSDK/libs/android/$(TARGET_ARCH_ABI)/libagora-rtc-sdk-jni.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := apm-plugin-agora-cocos2dx
LOCAL_SRC_FILES := ../../../../../AgoraGamingSDK/libs/android/$(TARGET_ARCH_ABI)/libapm-plugin-agora-cocos2dx.so
include $(PREBUILT_SHARED_LIBRARY)



include $(CLEAR_VARS)
$(call import-add-path,$(LOCAL_PATH)/../../../../cocos2d-x )
$(call import-add-path,$(LOCAL_PATH)/../../../../cocos2d-x/external)
$(call import-add-path,$(LOCAL_PATH)/../../../../cocos2d-x/cocos)

LOCAL_MODULE := cocos2djs_shared

LOCAL_MODULE_FILENAME := libcocos2djs

LOCAL_SRC_FILES := hellojavascript/main.cpp \
                                 ../../../Classes/AppDelegate.cpp \
                                 ../../../Classes/jsb_AgoraVideo.cpp
                                 
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../Classes \
					$(LOCAL_PATH)/../../../../../AgoraGamingSDK/include

LOCAL_STATIC_LIBRARIES := cocos2d_js_static agora-cocos2dx

LOCAL_SHARED_LIBRARIES := agora-rtc apm-plugin-agora-cocos2dx

LOCAL_EXPORT_CFLAGS := -DCOCOS2D_DEBUG=1 -DCOCOS2D_JAVASCRIPT

include $(BUILD_SHARED_LIBRARY)

$(call import-module, scripting/js-bindings/proj.android)



