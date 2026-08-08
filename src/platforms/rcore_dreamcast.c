/**********************************************************************************************
*
*   rcore_dreamcast - Functions to manage window, graphics device and inputs
*
*   PLATFORM: DREAMCAST
*       - Sega Dreamcast
*
*   LIMITATIONS:
*       - Limitation 01
*       - Limitation 02
*
*   POSSIBLE IMPROVEMENTS:
*       - Improvement 01
*       - Improvement 02
*
*   ADDITIONAL NOTES:
*       - Port done by Antonio Jose Ramos Marquez aka bigboss @psxdev		
*
*   CONFIGURATION:
*       #define RCORE_PLATFORM_CUSTOM_FLAG
*           Custom flag for rcore on target platform -not used-
*
*   DEPENDENCIES:
*       Dreamcast SDK KOS KOS-PORTS - Provides C API to access Dreamcast homebrew functionality
*
*
*   LICENSE: zlib/libpng
*
*   Copyright (c) 2013-2023 Ramon Santamaria (@raysan5) and contributors
*
*   This software is provided "as-is", without any express or implied warranty. In no event
*   will the authors be held liable for any damages arising from the use of this software.
*
*   Permission is granted to anyone to use this software for any purpose, including commercial
*   applications, and to alter it and redistribute it freely, subject to the following restrictions:
*
*     1. The origin of this software must not be misrepresented; you must not claim that you
*     wrote the original software. If you use this software in a product, an acknowledgment
*     in the product documentation would be appreciated but is not required.
*
*     2. Altered source versions must be plainly marked as such, and must not be misrepresented
*     as being the original software.
*
*     3. This notice may not be removed or altered from any source distribution.
*
**********************************************************************************************/
#include <kos.h>
#include <dc/maple/purupuru.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glkos.h>

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
extern CoreData CORE;                   // Global CORE state context

//----------------------------------------------------------------------------------
// Module Internal Functions Declaration
//----------------------------------------------------------------------------------
int InitPlatform(void);          // Initialize platform (graphics, inputs and more)
bool InitGraphicsDevice(void);   // Initialize graphics device

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
// NOTE: Functions declaration is provided by raylib.h

//----------------------------------------------------------------------------------
// Module Functions Definition: Window and Graphics Device
//----------------------------------------------------------------------------------

// Check if application should close
bool WindowShouldClose(void)
{
    if (CORE.Window.ready) return CORE.Window.shouldClose;
    else return true;
}

// Toggle fullscreen mode
void ToggleFullscreen(void)
{
    TRACELOG(LOG_WARNING, "ToggleFullscreen() not available on target platform");
}

// Toggle borderless windowed mode
void ToggleBorderlessWindowed(void)
{
    TRACELOG(LOG_WARNING, "ToggleBorderlessWindowed() not available on target platform");
}

// Set window state: maximized, if resizable
void MaximizeWindow(void)
{
    TRACELOG(LOG_WARNING, "MaximizeWindow() not available on target platform");
}

// Set window state: minimized
void MinimizeWindow(void)
{
    TRACELOG(LOG_WARNING, "MinimizeWindow() not available on target platform");
}

// Set window state: not minimized/maximized
void RestoreWindow(void)
{
    TRACELOG(LOG_WARNING, "RestoreWindow() not available on target platform");
}

// Set window configuration state using flags
void SetWindowState(unsigned int flags)
{
    (void)flags;
    TRACELOG(LOG_WARNING, "SetWindowState() not available on target platform");
}

// Clear window configuration state flags
void ClearWindowState(unsigned int flags)
{
    (void)flags;
    TRACELOG(LOG_WARNING, "ClearWindowState() not available on target platform");
}

// Set icon for window
void SetWindowIcon(Image image)
{
    (void)image;
    TRACELOG(LOG_WARNING, "SetWindowIcon() not available on target platform");
}

// Set icon for window
void SetWindowIcons(Image *images, int count)
{
    (void)images;
    (void)count;
    TRACELOG(LOG_WARNING, "SetWindowIcons() not available on target platform");
}

// Set title for window
void SetWindowTitle(const char *title)
{
    CORE.Window.title = title;
}

// Set window position on screen (windowed mode)
void SetWindowPosition(int x, int y)
{
    (void)x;
    (void)y;
    TRACELOG(LOG_WARNING, "SetWindowPosition() not available on target platform");
}

// Set monitor for the current window
void SetWindowMonitor(int monitor)
{
    (void)monitor;
    TRACELOG(LOG_WARNING, "SetWindowMonitor() not available on target platform");
}

// Set window minimum dimensions (FLAG_WINDOW_RESIZABLE)
void SetWindowMinSize(int width, int height)
{
    CORE.Window.screenMin.width = width;
    CORE.Window.screenMin.height = height;
}

// Set window maximum dimensions (FLAG_WINDOW_RESIZABLE)
void SetWindowMaxSize(int width, int height)
{
    CORE.Window.screenMax.width = width;
    CORE.Window.screenMax.height = height;
}

// Set window dimensions
void SetWindowSize(int width, int height)
{
    (void)width;
    (void)height;
    TRACELOG(LOG_WARNING, "SetWindowSize() not available on target platform");
}

// Set window opacity, value opacity is between 0.0 and 1.0
void SetWindowOpacity(float opacity)
{
    (void)opacity;
    TRACELOG(LOG_WARNING, "SetWindowOpacity() not available on target platform");
}

// Set window focused
void SetWindowFocused(void)
{
    TRACELOG(LOG_WARNING, "SetWindowFocused() not available on target platform");
}

// Get native window handle
void *GetWindowHandle(void)
{
    TRACELOG(LOG_WARNING, "GetWindowHandle() not implemented on target platform");
    return NULL;
}

// Get number of monitors
int GetMonitorCount(void)
{
    return 1;
}

// Get number of monitors
int GetCurrentMonitor(void)
{
    return 0;
}

// Get selected monitor position
Vector2 GetMonitorPosition(int monitor)
{
    (void)monitor;
    return (Vector2){ 0, 0 };
}

// Get selected monitor width (currently used by monitor)
int GetMonitorWidth(int monitor)
{
    if (monitor != 0) return 0;
    return (vid_mode != NULL)? vid_mode->width : (int)CORE.Window.display.width;
}

// Get selected monitor height (currently used by monitor)
int GetMonitorHeight(int monitor)
{
    if (monitor != 0) return 0;
    return (vid_mode != NULL)? vid_mode->height : (int)CORE.Window.display.height;
}

// Get selected monitor physical width in millimetres
int GetMonitorPhysicalWidth(int monitor)
{
    (void)monitor;
    return 0;
}

// Get selected monitor physical height in millimetres
int GetMonitorPhysicalHeight(int monitor)
{
    (void)monitor;
    return 0;
}

// Get selected monitor refresh rate
int GetMonitorRefreshRate(int monitor)
{
    if (monitor != 0) return 0;
    return ((vid_mode != NULL) && ((vid_mode->flags & VID_PAL) != 0) && (vid_mode->cable_type != CT_VGA))? 50 : 60;
}

// Get the human-readable, UTF-8 encoded name of the selected monitor
const char *GetMonitorName(int monitor)
{
    return (monitor == 0)? "Sega Dreamcast" : "";
}

// Get window position XY on monitor
Vector2 GetWindowPosition(void)
{
    TRACELOG(LOG_WARNING, "GetWindowPosition() not implemented on target platform");
    return (Vector2){ 0, 0 };
}

// Get window scale DPI factor for current monitor
Vector2 GetWindowScaleDPI(void)
{
    TRACELOG(LOG_WARNING, "GetWindowScaleDPI() not implemented on target platform");
    return (Vector2){ 1.0f, 1.0f };
}

// Set clipboard text content
void SetClipboardText(const char *text)
{
    (void)text;
    TRACELOG(LOG_WARNING, "SetClipboardText() not implemented on target platform");
}

// Get clipboard text content
// NOTE: returned string is allocated and freed by GLFW
const char *GetClipboardText(void)
{
    TRACELOG(LOG_WARNING, "GetClipboardText() not implemented on target platform");
    return NULL;
}

// Get clipboard image content (Dreamcast has no system clipboard)
Image GetClipboardImage(void)
{
    return (Image){ 0 };
}

// Show mouse cursor
void ShowCursor(void)
{
    CORE.Input.Mouse.cursorHidden = false;
}

// Hides mouse cursor
void HideCursor(void)
{
    CORE.Input.Mouse.cursorHidden = true;
}

// Enables cursor (unlock cursor)
void EnableCursor(void)
{
    // Set cursor position in the middle
    SetMousePosition(CORE.Window.screen.width/2, CORE.Window.screen.height/2);

    CORE.Input.Mouse.cursorHidden = false;
}

// Disables cursor (lock cursor)
void DisableCursor(void)
{
    // Set cursor position in the middle
    SetMousePosition(CORE.Window.screen.width/2, CORE.Window.screen.height/2);

    CORE.Input.Mouse.cursorHidden = true;
}

// Swap back buffer with front buffer (screen drawing)
void SwapScreenBuffer(void)
{
    glKosSwapBuffers();
}

//----------------------------------------------------------------------------------
// Module Functions Definition: Misc
//----------------------------------------------------------------------------------

// Get elapsed time measure in seconds since InitTimer()
double GetTime(void)
{
    double time = 0.0;
    struct timespec ts = { 0 };
    clock_gettime(CLOCK_MONOTONIC, &ts);
    unsigned long long int nanoSeconds = (unsigned long long int)ts.tv_sec*1000000000LLU + (unsigned long long int)ts.tv_nsec;
    time = (double)(nanoSeconds - CORE.Time.base)*1e-9;  // Elapsed time since InitTimer()

    return time;
}

// Open URL with default system browser (if available)
// NOTE: This function is only safe to use if you control the URL given.
// A user could craft a malicious string performing another action.
// Only call this function yourself not with user input or make sure to check the string yourself.
// Ref: https://github.com/raysan5/raylib/issues/686
void OpenURL(const char *url)
{
    if (url == NULL) return;

   // Security check to (partially) avoid malicious code on target platform
    if (strchr(url, '\'') != NULL) TRACELOG(LOG_WARNING, "SYSTEM: Provided URL could be potentially malicious, avoid [\'] character");
    else
    {
        // TODO:
    }
}

//----------------------------------------------------------------------------------
// Module Functions Definition: Inputs
//----------------------------------------------------------------------------------

// Set internal gamepad mappings
int SetGamepadMappings(const char *mappings)
{
    (void)mappings;
    TRACELOG(LOG_WARNING, "SetGamepadMappings() not implemented on target platform");
    return 0;
}

// Set gamepad vibration using the Jump Pack attached to the selected controller.
// Dreamcast exposes one physical motor; left/right amplitudes map to its two direction power fields.
void SetGamepadVibration(int gamepad, float leftMotor, float rightMotor, float duration)
{
    if ((gamepad < 0) || (gamepad >= MAX_GAMEPADS)) return;

    maple_device_t *controller = maple_enum_type(gamepad, MAPLE_FUNC_CONTROLLER);
    if ((controller == NULL) || !controller->valid) return;

    maple_device_t *rumble = NULL;
    for (int unit = 1; unit < MAPLE_UNIT_COUNT; unit++)
    {
        maple_device_t *device = maple_enum_dev(controller->port, unit);
        if ((device != NULL) && device->valid && ((device->info.functions & MAPLE_FUNC_PURUPURU) != 0))
        {
            rumble = device;
            break;
        }
    }

    if (rumble == NULL) return;

    if (!(leftMotor > 0.0f)) leftMotor = 0.0f; // Also normalizes NaN
    else if (leftMotor > 1.0f) leftMotor = 1.0f;
    if (!(rightMotor > 0.0f)) rightMotor = 0.0f; // Also normalizes NaN
    else if (rightMotor > 1.0f) rightMotor = 1.0f;

    purupuru_effect_t effect = { .motor = 1 };
    if ((duration > 0.0f) && ((leftMotor > 0.0f) || (rightMotor > 0.0f)))
    {
        unsigned int leftPower = (unsigned int)(leftMotor*7.0f + 0.5f);
        unsigned int rightPower = (unsigned int)(rightMotor*7.0f + 0.5f);
        if ((leftMotor > 0.0f) && (leftPower == 0)) leftPower = 1;
        if ((rightMotor > 0.0f) && (rightPower == 0)) rightPower = 1;

        float durationUnits = duration*2.0f; // KOS' inc=1 basic effect is approximately a 0.5 s jolt
        if (durationUnits < 1.0f) durationUnits = 1.0f;
        else if (durationUnits > 255.0f) durationUnits = 255.0f;

        effect.bpow = leftPower;
        effect.fpow = rightPower;
        effect.freq = 26;
        effect.inc = (uint8_t)(durationUnits + 0.5f);
    }

    int result = purupuru_rumble(rumble, &effect);
    if ((result != MAPLE_EOK) && (result != MAPLE_EAGAIN)) TRACELOG(LOG_WARNING, "INPUT: Dreamcast vibration request rejected (%i)", result);
}

// Set mouse position XY
void SetMousePosition(int x, int y)
{
    CORE.Input.Mouse.currentPosition = (Vector2){ (float)x, (float)y };
    CORE.Input.Mouse.previousPosition = CORE.Input.Mouse.currentPosition;
}

// Set mouse cursor
void SetMouseCursor(int cursor)
{
    (void)cursor;
    TRACELOG(LOG_WARNING, "SetMouseCursor() not implemented on target platform");
}

// Button mapping table
static const struct {
    uint16_t dcbutton;
    int raylibButton;
} buttonMap[] = {
    { CONT_START, GAMEPAD_BUTTON_MIDDLE_RIGHT },
    { CONT_A, GAMEPAD_BUTTON_RIGHT_FACE_DOWN },
    { CONT_B, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT },
    { CONT_X, GAMEPAD_BUTTON_RIGHT_FACE_LEFT },
    { CONT_Y, GAMEPAD_BUTTON_RIGHT_FACE_UP },
    { CONT_Z, GAMEPAD_BUTTON_LEFT_TRIGGER_1 },
    { CONT_D, GAMEPAD_BUTTON_RIGHT_TRIGGER_1 },
    { CONT_DPAD_UP, GAMEPAD_BUTTON_LEFT_FACE_UP },
    { CONT_DPAD_DOWN, GAMEPAD_BUTTON_LEFT_FACE_DOWN },
    { CONT_DPAD_LEFT, GAMEPAD_BUTTON_LEFT_FACE_LEFT },
    { CONT_DPAD_RIGHT, GAMEPAD_BUTTON_LEFT_FACE_RIGHT },
};

static void MapControls(int padIndex, cont_state_t *dcPadState) {
    // Iterate over the button map and update button states
    const int buttonCount = (int)(sizeof(buttonMap)/sizeof(buttonMap[0]));
    for (int j = 0; j < buttonCount; j++) {
        if (dcPadState->buttons & buttonMap[j].dcbutton) {
            CORE.Input.Gamepad.currentButtonState[padIndex][buttonMap[j].raylibButton] = 1;
            CORE.Input.Gamepad.lastButtonPressed = buttonMap[j].raylibButton;
        } else {
            CORE.Input.Gamepad.currentButtonState[padIndex][buttonMap[j].raylibButton] = 0;
        }
    }

    // Handle axis data
    CORE.Input.Gamepad.axisState[padIndex][GAMEPAD_AXIS_LEFT_X] = (float)dcPadState->joyx*(1.0f/128.0f);
    CORE.Input.Gamepad.axisState[padIndex][GAMEPAD_AXIS_LEFT_Y] = (float)dcPadState->joyy*(1.0f/128.0f);
    CORE.Input.Gamepad.axisState[padIndex][GAMEPAD_AXIS_RIGHT_X] = (float)dcPadState->joy2x*(1.0f/128.0f);
    CORE.Input.Gamepad.axisState[padIndex][GAMEPAD_AXIS_RIGHT_Y] = (float)dcPadState->joy2y*(1.0f/128.0f);
    CORE.Input.Gamepad.axisState[padIndex][GAMEPAD_AXIS_LEFT_TRIGGER] = (float)dcPadState->ltrig*(1.0f/255.0f);
    CORE.Input.Gamepad.axisState[padIndex][GAMEPAD_AXIS_RIGHT_TRIGGER] = (float)dcPadState->rtrig*(1.0f/255.0f);

    CORE.Input.Gamepad.axisCount[padIndex] = 6;
}

#if defined(__GNUC__) || defined(__clang__)
__attribute__((cold, noinline))
#endif
static void DisconnectGamepad(int padIndex)
{
    memcpy(CORE.Input.Gamepad.previousButtonState[padIndex], CORE.Input.Gamepad.currentButtonState[padIndex], sizeof(CORE.Input.Gamepad.currentButtonState[padIndex]));
    memset(CORE.Input.Gamepad.currentButtonState[padIndex], 0, sizeof(CORE.Input.Gamepad.currentButtonState[padIndex]));
    memset(CORE.Input.Gamepad.axisState[padIndex], 0, sizeof(CORE.Input.Gamepad.axisState[padIndex]));
    CORE.Input.Gamepad.axisCount[padIndex] = 0;
    CORE.Input.Gamepad.ready[padIndex] = false;
}

// Register all input events
void PollInputEvents(void)
{
#if defined(SUPPORT_GESTURES_SYSTEM)
    // NOTE: Gestures update must be called every frame to reset gestures correctly
    // because ProcessGestureEvent() is just called on an event, not every frame
    UpdateGestures();
#endif

    // Reset keys/chars pressed registered
    CORE.Input.Keyboard.keyPressedQueueCount = 0;
    CORE.Input.Keyboard.charPressedQueueCount = 0;

    // Reset key repeats
    for (int i = 0; i < MAX_KEYBOARD_KEYS; i++) CORE.Input.Keyboard.keyRepeatInFrame[i] = 0;

    // Reset last gamepad button/axis registered state
    CORE.Input.Gamepad.lastButtonPressed = 0; // GAMEPAD_BUTTON_UNKNOWN
    //CORE.Input.Gamepad.axisCount = 0;

    // Register previous touch states
    for (int i = 0; i < MAX_TOUCH_POINTS; i++) CORE.Input.Touch.previousTouchState[i] = CORE.Input.Touch.currentTouchState[i];

    maple_device_t *cont;
    cont_state_t *dcPadState;
    
    for (int padIndex = 0; padIndex < MAX_GAMEPADS; padIndex++)
    {
        cont = maple_enum_type(padIndex, MAPLE_FUNC_CONTROLLER);
        dcPadState = (cont != NULL)? (cont_state_t *)maple_dev_status(cont) : NULL;
        if (dcPadState == NULL)
        {
            /* Clear once on disconnect, not every frame for every empty port. */
            if (__builtin_expect(CORE.Input.Gamepad.ready[padIndex], 0)) DisconnectGamepad(padIndex);
            continue;
        }

        CORE.Input.Gamepad.ready[padIndex] = true;
        for (int k = 0; k < MAX_GAMEPAD_BUTTONS; k++)
            CORE.Input.Gamepad.previousButtonState[padIndex][k] = CORE.Input.Gamepad.currentButtonState[padIndex][k];
        MapControls(padIndex, dcPadState);
    }
}

//----------------------------------------------------------------------------------
// Module Internal Functions Definition
//----------------------------------------------------------------------------------

// Initialize platform: graphics, inputs and more
int InitPlatform(void)
{

    TRACELOG(LOG_INFO, "PLATFORM: calling dreamcast gl init");
    glKosInit();
    


    CORE.Window.fullscreen = true;
    CORE.Window.flags |= FLAG_FULLSCREEN_MODE;

    CORE.Window.display.width = CORE.Window.screen.width;            // User desired width
    CORE.Window.display.height = CORE.Window.screen.height;          // User desired height
    CORE.Window.render.width = CORE.Window.screen.width;
    CORE.Window.render.height = CORE.Window.screen.height;
    CORE.Window.eventWaiting = false;
    CORE.Window.screenScale = MatrixIdentity();     // No draw scaling required by default
    CORE.Window.currentFbo.width = CORE.Window.screen.width;
    CORE.Window.currentFbo.height = CORE.Window.screen.height;
    CORE.Input.Mouse.currentPosition.x = (float)CORE.Window.screen.width/2.0f;
    CORE.Input.Mouse.currentPosition.y = (float)CORE.Window.screen.height/2.0f;
    CORE.Input.Mouse.scale = (Vector2){ 1.0f, 1.0f };
    for (int padIndex = 0; padIndex < MAX_GAMEPADS; padIndex++)
        snprintf(CORE.Input.Gamepad.name[padIndex], sizeof(CORE.Input.Gamepad.name[padIndex]), "Dreamcast Controller %i", padIndex + 1);


    

    // At this point we need to manage render size vs screen size
    // NOTE: This function use and modify global module variables:
    //  -> CORE.Window.screen.width/CORE.Window.screen.height
    //  -> CORE.Window.render.width/CORE.Window.render.height
    //  -> CORE.Window.screenScale
    SetupFramebuffer(CORE.Window.display.width, CORE.Window.display.height);

    //ANativeWindow_setBuffersGeometry(platform.app->window, CORE.Window.render.width, CORE.Window.render.height, displayFormat);
    //ANativeWindow_setBuffersGeometry(platform.app->window, 0, 0, displayFormat);       // Force use of native display size

    
    {
        CORE.Window.render.width = CORE.Window.screen.width;
        CORE.Window.render.height = CORE.Window.screen.height;
        CORE.Window.currentFbo.width = CORE.Window.render.width;
        CORE.Window.currentFbo.height = CORE.Window.render.height;

        TRACELOG(LOG_INFO, "PLATFORM: Device initialized successfully");
        TRACELOG(LOG_INFO, "    > Display size: %i x %i", CORE.Window.display.width, CORE.Window.display.height);
        TRACELOG(LOG_INFO, "    > Screen size:  %i x %i", CORE.Window.screen.width, CORE.Window.screen.height);
        TRACELOG(LOG_INFO, "    > Render size:  %i x %i", CORE.Window.render.width, CORE.Window.render.height);
        TRACELOG(LOG_INFO, "    > Viewport offsets: %i, %i", CORE.Window.renderOffset.x, CORE.Window.renderOffset.y);
    }

    // Load OpenGL extensions
    // NOTE: GL procedures address loader is required to load extensions
    //rlLoadExtensions(eglGetProcAddress);
    //const char *gl_exts = (char *) glGetString(GL_EXTENSIONS);
    //TRACELOG(LOG_INFO,"PLATFORM: GL_VENDOR:   \"%s\"", glGetString(GL_VENDOR));
    //TRACELOG(LOG_INFO,"PLATFORM: GL_VERSION:  \"%s\"", glGetString(GL_VERSION));
    //TRACELOG(LOG_INFO,"PLATFORM: GL_RENDERER: \"%s\"", glGetString(GL_RENDERER));
    //TRACELOG(LOG_INFO,"PLATFORM: SL_VERSION:  \"%s\"", glGetString(GL_SHADING_LANGUAGE_VERSION));
    CORE.Window.ready = true;

    // Initialize hi-res timer
    InitTimer();

    // Initialize base path for storage
    CORE.Storage.basePath = GetWorkingDirectory();
    TRACELOG(LOG_INFO, "PLATFORM: Initialized");

    return 0;
}

// Close platform
void ClosePlatform(void)
{
    glKosShutdown();
}

// EOF
