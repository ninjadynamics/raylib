#include <math.h>
#include <stdio.h>

#define RGESTURES_STANDALONE
#define RGESTURES_IMPLEMENTATION
#include "../src/rgestures.h"

static int failures = 0;

static void expect_true(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL: %s\n", message);
        failures++;
    }
}

static void expect_near(float actual, float expected, const char *message)
{
    if (fabsf(actual - expected) > 0.0001f)
    {
        fprintf(stderr, "FAIL: %s (got %.6f, expected %.6f)\n", message, actual, expected);
        failures++;
    }
}

int main(void)
{
    GestureEvent event = { 0 };
    event.touchAction = TOUCH_ACTION_DOWN;
    event.pointCount = 2;
    event.position[0] = (Vector2){ 0.2f, 0.5f };
    event.position[1] = (Vector2){ 0.8f, 0.5f };
    ProcessGestureEvent(event);

    event.touchAction = TOUCH_ACTION_MOVE;
    event.position[0] = (Vector2){ 0.3f, 0.5f };
    event.position[1] = (Vector2){ 0.7f, 0.5f };
    ProcessGestureEvent(event);

    Vector2 pinch = GetGesturePinchVector();
    expect_near(pinch.x, 0.4f, "pinch vector uses current positions");
    expect_near(pinch.y, 0.0f, "pinch vector y remains stable");
    expect_true(GetGestureDetected() == GESTURE_PINCH_IN, "first inward move detects pinch-in");

    // Repeating the same positions must compare against the preceding move, not stale down coordinates.
    ProcessGestureEvent(event);
    expect_true(GetGestureDetected() == GESTURE_HOLD, "stationary second move does not repeat stale pinch");

    event.touchAction = TOUCH_ACTION_CANCEL;
    ProcessGestureEvent(event);
    pinch = GetGesturePinchVector();
    expect_true(GetGestureDetected() == GESTURE_NONE, "cancel clears current gesture");
    expect_near(pinch.x, 0.0f, "cancel clears stale pinch x");
    expect_near(pinch.y, 0.0f, "cancel clears stale pinch y");
    expect_true(!IsGestureDetected(GESTURE_NONE), "GESTURE_NONE is not reported as a detected gesture");

    return (failures == 0)? 0 : 1;
}
