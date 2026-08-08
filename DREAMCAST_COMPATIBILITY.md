# Dreamcast compatibility no-ops

This file records raylib and rlgl entry points that deliberately remain callable on
Dreamcast even though the platform cannot implement their full desktop semantics.
Keeping the symbols and signatures is intentional: portable raylib code can compile and
link unchanged, and an unsupported request degrades to a documented no-op or neutral
sentinel instead of being removed from the API.

The inventory applies to `PLATFORM_DREAMCAST` with the OpenGL 1.1/GLdc backend. A
**no-op** ignores the requested external effect. A **neutral query** returns a stable
sentinel. A **state-only shim** updates raylib bookkeeping but cannot change the fixed
console hardware. Partially supported APIs list the exact unsupported cases.

Compile-time-disabled modules are not no-ops. In particular, the default Dreamcast build
does not enable `SUPPORT_MODULE_RAUDIO`, `SUPPORT_GESTURES_SYSTEM`, or
`SUPPORT_AUTOMATION_EVENTS`. The declarations remain source-compatible, but disabled
module symbols are not promised by the resulting archive. Gestures and automation are
real implementations when explicitly enabled; without a Dreamcast mouse/touch producer,
gesture state remains neutral unless an application injects events. `raudio` is different:
miniaudio's available backends currently require POSIX facilities KOS does not provide, so
`SUPPORT_MODULE_RAUDIO` is not a supported Dreamcast opt-in. Applications must use their
platform/game audio path until a KOS-native miniaudio backend exists.

## Window, desktop integration, and clipboard

The Dreamcast has one fixed full-screen video output, no desktop window manager, no
system clipboard, and no process-wide URL handler.

| API | Dreamcast behavior | Kind / reason |
|---|---|---|
| `ToggleFullscreen()` | Leaves the fixed full-screen output unchanged; logs unavailable. | No-op; there is no windowed mode. |
| `ToggleBorderlessWindowed()` | Leaves display state unchanged; logs unavailable. | No-op; there is no desktop window. |
| `MaximizeWindow()`, `MinimizeWindow()`, `RestoreWindow()` | Leave display state unchanged; log unavailable. | No-op; there is no window manager. |
| `SetWindowState()`, `ClearWindowState()` | Ignore the requested mutable window flags; log unavailable. | No-op for post-init desktop window state. |
| `SetWindowIcon()`, `SetWindowIcons()` | Ignore icon data; log unavailable. | No-op; the console has no desktop icon. |
| `SetWindowPosition()`, `SetWindowMonitor()`, `SetWindowSize()` | Leave the fixed video output unchanged; log unavailable. | No-op; monitor, position, and mode are fixed by platform/video initialization. |
| `SetWindowOpacity()`, `SetWindowFocused()` | Leave display state unchanged; log unavailable. | No-op; those desktop concepts do not exist. |
| `SetWindowTitle()` | Updates `CORE.Window.title` only. | State-only shim; there is no native title bar. |
| `SetWindowMinSize()`, `SetWindowMaxSize()` | Update raylib's stored limits only. | State-only shim; the display is not resizable. |
| `GetWindowHandle()` | Returns `NULL` and logs unimplemented. | Neutral query; there is no native window object. |
| `GetMonitorPosition()` | Returns `{ 0, 0 }` for the single fixed output. | Neutral query; the console has no desktop monitor coordinate space. |
| `GetWindowPosition()` | Returns `{ 0, 0 }` and logs unimplemented. | Neutral query for the single fixed display. |
| `GetWindowScaleDPI()` | Returns `{ 1, 1 }` and logs unimplemented. | Neutral query; no desktop DPI scaling is applied. |
| `GetMonitorPhysicalWidth()`, `GetMonitorPhysicalHeight()` | Return `0`. | Neutral query; KOS video mode data does not expose TV dimensions. |
| `SetClipboardText()` | Ignores text and logs unimplemented. | No-op; there is no system clipboard. |
| `GetClipboardText()` | Returns `NULL` and logs unimplemented. | Neutral query. |
| `GetClipboardImage()` | Returns an exactly zero-initialized `Image`. | Neutral query. `SUPPORT_CLIPBOARD_IMAGE` is disabled so this shim does not pull image decoders into the ELF. |
| `OpenURL()` | Performs the existing quote safety check, then launches nothing. | No-op; there is no default browser/URL service. |

The console initializes as full-screen and `IsWindowFullscreen()` reports true. Other
`IsWindowState()`/hidden/minimized/maximized/focused queries read raylib's stored flags;
for desktop-only flags that is configuration bookkeeping, not evidence of a native window
manager effect.

The remaining monitor queries are functional for Dreamcast's single fixed output:
`GetMonitorCount()` returns `1`, `GetCurrentMonitor()` returns `0`,
`GetMonitorName(0)` returns `"Sega Dreamcast"`, width and height come from the active KOS
video mode, and refresh rate reports `50` Hz for non-VGA PAL or `60` Hz otherwise. Invalid
monitor indices return the documented neutral zero/empty values.

`SetConfigFlags()` preserves the caller's bits, but the Dreamcast platform initializer
only enforces the console's full-screen state; it does not translate desktop window,
resize, decoration, focus, opacity, high-DPI, mouse-passthrough, display-mode, MSAA, or
swap-interval hints into GLdc/KOS configuration. Queries can therefore reflect requested bits whose
hardware effect is a compatibility no-op.

## Input compatibility

| API | Dreamcast behavior | Kind / reason |
|---|---|---|
| `SetGamepadMappings()` | Ignores the mapping string, logs unimplemented, and returns `0`. | No-op; the backend has a fixed Dreamcast-controller mapping. |
| `SetMouseCursor()` | Ignores the requested desktop cursor shape and logs unimplemented. | No-op; no system cursor compositor exists. |
| `ShowCursor()`, `HideCursor()`, `EnableCursor()`, `DisableCursor()` | Update raylib's `cursorHidden` bookkeeping; enable/disable also recenter the stored mouse position. | State-only shims; there is no native cursor to show, hide, lock, or release. |
| `SetMousePosition()`, `SetMouseOffset()`, `SetMouseScale()` | Update raylib's synthetic mouse state only. | State-only shims while the backend has no mouse-state producer. |

`SetGamepadVibration()` is **not** a no-op. It performs bounded, on-demand KOS Jump Pack
submission. It returns without an effect when the gamepad index is invalid, the controller
is absent, or no vibration pack is attached; those are ordinary device-availability
outcomes rather than an unsupported API.

Keyboard, mouse, and touch queries remain present, but this backend currently polls only
Dreamcast controllers. With no producer for those device states, their ordinary raylib
queries remain neutral (not pressed, zero movement, and zero touches).

## Blend modes

PowerVR2/GLdc exposes one source factor and one destination factor with an additive
equation:

```text
result = source*sourceFactor + destination*destinationFactor
```

`rlSetBlendMode()`, `rlSetBlendFactors()`, and `rlSetBlendFactorsSeparate()` are silent
compatibility no-ops for every mode on Dreamcast. Consequently the public
`BeginBlendMode()`/`EndBlendMode()` pair is also a no-op. The calls do not flush, retain
state, allocate, log, or substitute an approximation. Native Dreamcast render helpers
configure the PVR/GLdc blend state directly where an effect actually requires it.

This preserves the established raylib-dc contract: portable blend brackets remain
callable without changing renderer-owned PVR state or adding work to a frame. PowerVR2
also cannot express subtract equations or separate RGB/alpha factors in one pass.

Multipass or framebuffer approximations are intentionally not used: they would change
results and violate the backend's zero-regression performance contract.

## Fixed-function raster controls

| rlgl API | Dreamcast behavior | Kind / reason |
|---|---|---|
| `rlEnableWireMode()` | Does nothing. | No-op; GLdc's `glPolygonMode()` is a no-op and PowerVR2 has no polygon line raster mode. |
| `rlEnablePointMode()` | Does nothing. | No-op; polygon point raster mode is unavailable. |
| `rlDisableWireMode()` | Does nothing. | Paired compatibility no-op. |
| `rlEnableSmoothLines()`, `rlDisableSmoothLines()` | Do nothing. | No-op; GLdc does not implement `GL_LINE_SMOOTH`. |
| `rlColorMask()` | Ignores all four channel-enable flags. | No-op; GLdc's matching entry point has no hardware effect. The raylib shim also avoids a needless batch flush. |

`rlSetLineWidth()` forwards the value to GLdc and `rlGetLineWidth()` returns its mirror
because GLdc does not expose `GL_LINE_WIDTH` through `glGetFloatv()`. Raylib's own
`RL_LINES` path is discarded as described below, so line width only matters to
backend-specific direct line submission.

Dreamcast captures `RL_LINES` as a silent discard mode. Position vertices inside the
matching `rlBegin()`/`rlEnd()` pair are ignored without buffering or submission, queued
triangle/quad traffic is not flushed, and no GLdc line primitive reaches a hazardous render list.
Color and texture-coordinate setters still update their ordinary current-state mirrors;
captured normals have the no-op behavior documented below. Raw rlgl line calls and the
raylib helpers backed by them therefore remain callable no-ops, including:

- `DrawLine()`, `DrawLineV()`, and `DrawLineStrip()`;
- `DrawCircleSectorLines()`, `DrawCircleLines()`, `DrawCircleLinesV()`,
  `DrawEllipseLines()`, and `DrawRingLines()`;
- `DrawRectangleLines()` and the thin `RL_LINES` branch of
  `DrawRectangleRoundedLines()`;
- `DrawTriangleLines()` and `DrawPolyLines()`;
- `DrawLine3D()`, `DrawPoint3D()`, `DrawCircle3D()`, `DrawCubeWires()`,
  `DrawCubeWiresV()`, `DrawSphereWires()`, `DrawCylinderWires()`,
  `DrawCylinderWiresEx()`, `DrawCapsuleWires()`, `DrawRay()`, and the
  `DrawBoundingBox()` wrapper.

Some high-level helpers still calculate their vertices before the low-level discard; they
allocate nothing and submit nothing, but should not be called speculatively in hot loops.
Area-geometry alternatives such as `DrawLineEx()`, `DrawRectangleLinesEx()`,
`DrawPolyLinesEx()`, and the Dreamcast `DrawGrid()` implementation remain functional.

At the public model layer, `DrawModelWires()`, `DrawModelWiresEx()`,
`DrawModelPoints()`, and `DrawModelPointsEx()` are explicit, silent no-ops on Dreamcast.
They formerly called the ordinary filled-model draw after a no-op polygon-mode request,
which was a visually incorrect approximation. Portable callers can retain the calls;
Dreamcast-specific visuals must instead supply explicitly authored line/point geometry.

## Mesh, model, material, and animation shims

`DrawMesh()` and the ordinary `DrawModel*()` path are functional fixed-function partial
implementations. They consume positions, primary texture coordinates, normals, native
BGRA vertex colors, and `MATERIAL_MAP_DIFFUSE` texture/color. The following retained data
has no rendering effect in that path:

- `material.shader` and every non-diffuse material map, including normal, specular, PBR,
  emission, occlusion, irradiance, prefilter, and cubemap maps;
- mesh tangents and secondary texture coordinates;
- GPU bone IDs, weights, and bone matrices.

`SetMaterialTexture()` still stores every map for source/API compatibility, but setting a
non-diffuse map is a state-only operation on Dreamcast. Effects that need these channels
must be baked into diffuse/vertex data or implemented as explicit backend-native passes.

`UploadMesh()` is also a partial shim, not a no-op. Dreamcast draws retained CPU client
arrays, so upload creates bookkeeping (and DCMesh linkage where applicable) rather than
VAO/VBO storage. `UpdateMeshBuffer()` is functional: it validates the requested range and
updates the retained CPU stream and matching DCMesh data.

DCMesh sidecars store source glTF material indices. raylib inserts its default material
at slot 0 and therefore loads glTF material `N` at raylib slot `N + 1`; an unmaterialed
primitive remains on slot 0. The sidecar validator accepts both the direct slot-0 case and
the glTF-to-raylib `+1` mapping. Rejecting that offset disables the strip/batch fast path
for otherwise valid models and is not a compatible validation policy.

`LoadModelAnimations()` and `UpdateModelAnimation()` retain raylib's CPU animation data
and calculations, but ordinary Dreamcast drawing reads the base `mesh.vertices` and
`mesh.normals` arrays while `rlUpdateVertexBuffer()` is inert. Consequently the standard
animation update has no visual effect and is **not** a cheap no-op—it can still perform
the full CPU skinning work. A Dreamcast animation path must skin from preserved base data
into `mesh.vertices`/`mesh.normals` and call `UpdateMeshBuffer()`, or use baked model
frames.

## OpenGL 1.1 feature-family shims

These are standard rlgl compatibility stubs in an OpenGL 1.1 backend. They are retained so
shared low-level code can compile, but callers must use the listed Dreamcast path when they
need an effect.

### Programmable shaders

| APIs | Dreamcast result | Alternative |
|---|---|---|
| `rlEnableShader()`, `rlDisableShader()`, `rlUnloadShaderProgram()`, `rlSetUniform()`, `rlSetUniformMatrix()`, `rlSetUniformMatrices()`, `rlSetUniformSampler()`, `rlSetVertexAttributeDefault()`, `rlSetShader()` | No-op. | GLdc fixed-function state and CPU-side transforms/lighting. |
| `rlLoadShaderCode()`, `rlCompileShader()`, `rlLoadShaderProgram()` | Return `0`. | No programmable shader stage exists on PowerVR2. |
| `rlGetShaderIdDefault()` | Returns `0`. | Fixed-function pipeline has no shader object. |
| `rlGetShaderLocsDefault()` | Returns `NULL`. | Not applicable. |
| `rlGetLocationUniform()`, `rlGetLocationAttrib()` | Return `-1`. | Not applicable. |

The public raylib shader facade inherits those neutral results:

| APIs | Dreamcast result |
|---|---|
| `LoadShader()`, `LoadShaderFromMemory()` | Return a zero-initialized `Shader`. `LoadShader()` can still read the named source files before the backend rejects the shader. |
| `IsShaderValid()` | Returns `false` for the returned zero shader. |
| `GetShaderLocation()`, `GetShaderLocationAttrib()` | Return `-1`. |
| `BeginShaderMode()`, `EndShaderMode()`, `SetShaderValue()`, `SetShaderValueV()`, `SetShaderValueMatrix()`, `SetShaderValueTexture()` | No-op for the neutral shader/location values above. |
| `UnloadShader()` | Safe no-op for the neutral shader. |

### VAOs, VBOs, attributes, and instancing

| APIs | Dreamcast result | Alternative |
|---|---|---|
| `rlLoadVertexArray()`, `rlLoadVertexBuffer()`, `rlLoadVertexBufferElement()` | Return `0`. | CPU arrays through `rlEnableStatePointer()` or DCMesh. |
| `rlEnableVertexArray()` | Returns `false`. | Client arrays. |
| `rlDisableVertexArray()`, `rlEnableVertexBuffer()`, `rlDisableVertexBuffer()`, `rlEnableVertexBufferElement()`, `rlDisableVertexBufferElement()`, `rlUpdateVertexBuffer()`, `rlUpdateVertexBufferElements()`, `rlEnableVertexAttribute()`, `rlDisableVertexAttribute()`, `rlSetVertexAttribute()`, `rlSetVertexAttributeDivisor()`, `rlUnloadVertexArray()`, `rlUnloadVertexBuffer()` | No-op. | `UploadMesh()` retains CPU streams; `UpdateMeshBuffer()` copies validated ranges on Dreamcast. |
| `rlDrawVertexArrayInstanced()`, `rlDrawVertexArrayElementsInstanced()` | No-op. | Issue ordinary draws or use DCMesh batching. |
| `DrawMeshInstanced()` | No-op because its implementation requires the programmable/VBO backend. | Ordinary `DrawMesh()`/DCMesh batch submission. |

Non-instanced `rlDrawVertexArray()` and `rlDrawVertexArrayElements()` are supported through
GLdc client arrays. `rlEnableStatePointer()`/`rlDisableStatePointer()` are also supported.

### Cubemaps and multitexture

| APIs | Dreamcast result | Alternative |
|---|---|---|
| `rlActiveTextureSlot()` | No-op. | The raylib Dreamcast path uses the single `GL_TEXTURE_2D` unit. |
| `rlEnableTextureCubemap()`, `rlDisableTextureCubemap()`, `rlCubemapParameters()` | No-op. | Use supported 2D textures or a platform-specific effect. |
| `rlLoadTextureCubemap()` | Returns `0`. | Preprocess the effect into supported 2D assets. |
| `rlLoadTextureDepth()` | Returns `0`. | Dreamcast does not expose raylib's GL framebuffer/depth-texture route. |

The public `LoadTextureCubemap()` wrapper can still perform its CPU-side layout conversion
before `rlLoadTextureCubemap()` returns `0`; the returned cubemap is not valid. It is an
unsupported compatibility path, not a cheap no-op, so portable code should capability-gate
it rather than call it speculatively.

For ordinary 2D textures, `rlTextureParameters()` supports GLdc's normal wrap and
minification/magnification filter values. Requests for mirror-clamp wrapping,
anisotropic filtering, or mipmap LOD bias are compatibility no-ops because those
OpenGL extension controls are unavailable in this backend. They return before any batch
barrier or texture bind, so unsupported filtering requests have no render-thread cost. At
the public layer this means
`SetTextureWrap(texture, TEXTURE_WRAP_MIRROR_CLAMP)` and
`SetTextureFilter(texture, TEXTURE_FILTER_ANISOTROPIC_4X/8X/16X)` retain their symbols but
cannot apply the requested extension state.

### Framebuffer objects and render textures

| APIs | Dreamcast result | Alternative |
|---|---|---|
| `rlEnableFramebuffer()`, `rlDisableFramebuffer()`, `rlBlitFramebuffer()`, `rlBindFramebuffer()`, `rlActiveDrawBuffers()`, `rlFramebufferAttach()`, `rlUnloadFramebuffer()` | No-op. | GLdc's Dreamcast-specific flush-to-texture APIs where their stricter contract is acceptable. |
| `rlGetActiveFramebuffer()`, `rlLoadFramebuffer()` | Return `0`. | Main framebuffer or platform-specific render-to-texture path. |
| `rlFramebufferComplete()` | Returns `false`. | Not applicable without an FBO. |

At the public raylib layer, `LoadRenderTexture()` consequently returns a zero-initialized,
invalid `RenderTexture2D`, `IsRenderTextureValid()` returns `false`, and
`UnloadRenderTexture()` is a safe no-op for that value.

`BeginTextureMode()` is deliberately **not** classified as a no-op. Its framebuffer bind
does nothing, but the shared implementation still changes the viewport, projection, and
`CORE.Window` framebuffer bookkeeping; drawing therefore continues to the main framebuffer
under the requested render-texture dimensions. `EndTextureMode()` restores the main-screen
state. Portable code may keep these calls compiled, but it must treat an invalid result from
`LoadRenderTexture()` as the capability check and avoid entering texture mode.

### Pixel readback and screenshots

GLdc retains `glGetTexImage()` and `glReadPixels()` entry points but does not implement
their readback effects. The raylib/rlgl Dreamcast contract therefore uses explicit neutral
results instead of returning invented or uninitialized pixels:

| APIs | Dreamcast result |
|---|---|
| `rlReadTexturePixels()`, `rlReadScreenPixels()` | Return `NULL` without flushing or allocating. |
| `LoadImageFromTexture()`, `LoadImageFromScreen()` | Return a zero-initialized `Image` and log unsupported readback. |
| `TakeScreenshot()` | Performs its normal filename checks, receives `NULL` from screen readback, logs failure, and writes no file. |

### Generic render-batch bookkeeping

Dreamcast uses `rlgl_dc_batch.h`, a native 24-byte P3F/T2F/BGRA immediate batcher, instead
of the shader/VBO render-batch implementation.

`rlNormal3f()` is a compatibility no-op while an immediate triangle/quad/line block is
captured. The batch deliberately has no normal field or sidecar: adding one costs three
extra stores per vertex, increases the SH4 hot-path code, and moves GLdc out of the proven
P3F/T2F/BGRA submission lane. This does not remove mesh normals. The ordinary `DrawMesh()`
CPU client-array path and DCMesh path continue to consume their supported normal data;
an uncaptured direct OpenGL 1.1 `rlNormal3f()` call still forwards to `glNormal3f()`.

| APIs | Dreamcast result |
|---|---|
| `rlLoadRenderBatch()` | Returns a zero-initialized batch. |
| `rlUnloadRenderBatch()`, `rlDrawRenderBatch()`, `rlSetRenderBatchActive()` | No-op. |
| `rlCheckRenderBatchLimit()` | Returns `false`; the Dreamcast batcher owns its capacity checks. |

`rlDrawRenderBatchActive()` is **not** a no-op on Dreamcast: it flushes the native batch.

### Framebuffer bookkeeping and stereo

| APIs | Dreamcast result |
|---|---|
| `rlSetFramebufferWidth()`, `rlSetFramebufferHeight()` | Update Dreamcast rlgl bookkeeping only; they do not create or resize a hardware framebuffer. |
| `rlGetFramebufferWidth()`, `rlGetFramebufferHeight()` | Return the stored dimensions, initialized from `rlglInit()` and changed by the matching setters. |
| `rlGetTextureIdDefault()` | Returns `0`; the normal default-font configuration installs a shapes texture explicitly rather than relying on a shader default texture. |
| `rlEnableStereoRender()`, `rlDisableStereoRender()` | No-op. |
| `rlIsStereoRenderEnabled()` | Returns `false`. |
| `rlSetMatrixProjectionStereo()`, `rlSetMatrixViewOffsetStereo()` | No-op. |
| `rlGetMatrixProjectionStereo()`, `rlGetMatrixViewOffsetStereo()` | Return identity matrices. |

The matching public VR facade is neutral on Dreamcast:

| APIs | Dreamcast result |
|---|---|
| `LoadVrStereoConfig()` | Returns a zero-initialized `VrStereoConfig` and logs that the simulator is unsupported on OpenGL 1.1. |
| `BeginVrStereoMode()`, `EndVrStereoMode()` | No-op because all stereo state/matrix setters are compatibility stubs. |
| `UnloadVrStereoConfig()` | Logs the generic unimplemented message and otherwise does nothing; there is no backend resource to release. |

### Modern internal-matrix helpers

The ordinary OpenGL 1.1 matrix APIs (`rlMatrixMode()`, `rlPushMatrix()`,
`rlPopMatrix()`, `rlLoadIdentity()`, `rlTranslatef()`, `rlRotatef()`, `rlScalef()`,
`rlMultMatrixf()`, `rlFrustum()`, and `rlOrtho()`) are supported. The following
shader-backend bookkeeping helpers remain neutral compatibility shims:

| APIs | Dreamcast result |
|---|---|
| `rlGetMatrixTransform()` | Returns identity; GLdc owns the live OpenGL 1.1 matrix stack. |
| `rlSetMatrixModelview()`, `rlSetMatrixProjection()` | No-op. Use the ordinary matrix APIs above. |
| `rlLoadDrawQuad()`, `rlLoadDrawCube()` | No-op; these helpers are implemented only by the programmable/VBO backend. |

### Extension loading

`rlLoadExtensions(loader)` is a no-op in the Dreamcast OpenGL 1.1 build. GLdc is linked
directly and does not use a run-time OpenGL procedure loader, so the `loader` argument is
ignored and no extension-capability flags are populated through this entry point.

### Compute shaders, SSBOs, and image binding

| APIs | Dreamcast result |
|---|---|
| `rlLoadComputeShaderProgram()`, `rlLoadShaderBuffer()` | Return `0` and log that the GL 4.3 feature is unavailable. |
| `rlComputeShaderDispatch()`, `rlUnloadShaderBuffer()`, `rlUpdateShaderBuffer()`, `rlReadShaderBuffer()`, `rlBindShaderBuffer()`, `rlCopyShaderBuffer()` | No-op; `rlUnloadShaderBuffer()` logs unavailable. |
| `rlGetShaderBufferSize()` | Returns `0`. |
| `rlBindImageTexture()` | No-op and logs unavailable. |

## Maintenance rule

An intentional Dreamcast compatibility no-op must:

1. retain the public declaration and C symbol;
2. have deterministic neutral behavior with no hidden allocation, polling, or background work;
3. avoid pretending that a different visual operation is equivalent;
4. be listed here in the same change;
5. remain warning-clean in the Dreamcast build.

Existing diagnostic logging is stated explicitly in the tables above. New no-ops on a
potentially hot path must stay silent so compatibility does not create a performance trap.

If a capability later becomes available without regressing performance, implement it and
remove or narrow its entry here. Do not delete the API merely because the current hardware
backend cannot honor it.
