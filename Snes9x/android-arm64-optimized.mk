# ARM64 Optimized Build Configuration for Snes9x EX Plus
# This makefile is specifically optimized for ARM64 architecture builds

metadata_confDeps := ../EmuFramework/metadata/conf.mk

# ARM64 specific optimizations
android_arch := arm64
ANDROID_ABI := arm64-v8a

# Compiler optimizations for ARM64
CPPFLAGS += -DARM64_OPTIMIZED=1
CFLAGS += -march=armv8-a -mtune=cortex-a73 -O3 -flto -ffast-math
CXXFLAGS += -march=armv8-a -mtune=cortex-a73 -O3 -flto -ffast-math

# Link-time optimization
LDFLAGS += -flto -Wl,--gc-sections -Wl,--strip-all

# ARM64 NEON optimizations
CPPFLAGS += -DUSE_NEON=1 -mfpu=neon

# Build only ARM64 architecture
android_noArmv6 := 1
android_noArmv7 := 1
android_noX86 := 1
android_noX86_64 := 1

include $(IMAGINE_PATH)/make/shortcut/meta-builds/android-release.mk