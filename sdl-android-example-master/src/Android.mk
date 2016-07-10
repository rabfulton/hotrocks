LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

FREETYPE_PATH  := ../freetype
SDL_PATH       := ../SDL
SDL_IMAGE_PATH := ../SDL_image
SDL_MIXER_PATH := ../SDL_mixer
SDL_TTF_PATH   := ../SDL_ttf

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/$(SDL_PATH)/include \
	$(LOCAL_PATH)/$(SDL_IMAGE_PATH)/include \
	$(LOCAL_PATH)/$(SDL_MIXER_PATH)/include \
	$(LOCAL_PATH)/$(FREETYPE_PATH)/include \
	$(LOCAL_PATH)/$(SDL_TTF_PATH)/include

# Add your application source files here...
LOCAL_SRC_FILES := libs/SDL/src/main/android/SDL_android_main.c \
	$(subst $(LOCAL_PATH)/,,$(wildcard $(LOCAL_PATH)/*.c))

LOCAL_SHARED_LIBRARIES := SDL2 SDL2_image SDL2_mixer SDL2_ttf
LOCAL_CFLAGS += -std=c99
LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -llog

include $(BUILD_SHARED_LIBRARY)
