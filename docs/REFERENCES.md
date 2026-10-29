# References checked while scoping and implementing the prototype

- Meta OpenXR support for Quest: documents Android OpenXR loader requirements, the `XR_KHR_android_create_instance` setup, and the immersive HMD intent filter. https://developers.meta.com/vr/documentation/native/android/mobile-openxr/
- Meta OpenXR instance/session documentation: describes Android platform structures and OpenGL ES graphics binding setup. https://developers.meta.com/vr/documentation/native/android/mobile-openxr-instance-session/
- Khronos OpenXR Android loader on Maven Central: https://central.sonatype.com/artifact/org.khronos.openxr/openxr_loader_for_android
- Khronos OpenXR 1.1.65 specification and Android loader structures: https://registry.khronos.org/OpenXR/specs/1.1/html/xrspec.html
- Android NDK media APIs (`AMediaCodec`, `AMediaFormat`, `AImageReader`, `AImage`) are used for the Quest-side bring-up decoder; device decoder output format/performance still needs testing. https://developer.android.com/ndk/reference/group/media
- Meta OpenXR SDK samples (including action bindings, session lifecycle and controller examples): https://developers.meta.com/vr/documentation/native/native-openxr-sdk-sample/
- Meta Horizon Link documentation: Meta documents a SteamVR-free path for compatible PC development applications, but that uses Meta's runtime rather than this custom runtime. https://developers.meta.com/vr/documentation/unity/unity-link/
- ALVR repository: https://github.com/alvr-org/ALVR
- Monado Windows build notes: https://monado.pages.freedesktop.org/monado/winbuild.html

The above references inform API usage and architecture; QuestBridge XR is not a fork of those projects.
