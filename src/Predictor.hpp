#pragma once
#include <Geode/Geode.hpp>

// STEP 1: no prediction exists yet. This always says "unavailable",
// so the mod passes every press through immediately.
// Step 2 will replace the body with a sandboxed simulation.
enum class Verdict { Unavailable, Safe, Unsafe };

inline Verdict predictSurvival(GJBaseGameLayer*, bool /*isPlayer1*/, int /*delayTicks*/) {
	return Verdict::Unavailable;
}
