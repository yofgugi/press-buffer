#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include "Predictor.hpp"

using namespace geode::prelude;

// ---- shared state ----------------------------------------------------------
// Units: 1 "frame" = 1 displayed frame at 60 FPS = (TPS / 60) physics ticks.
// At 240 TPS that is 4 ticks per frame. ticksPerFrame is MEASURED, not assumed.
namespace buf {
	struct Pending { bool p1; int ticksLeft; int total; };

	inline std::vector<Pending> queue;
	inline constexpr int kWindows[5] = {1, 2, 4, 8, 16};
	inline int windowIdx = 2;          // starts at 4 frames
	inline int ticksPerFrame = 4;      // default until measured
	inline int tickCount = 0, frameCount = 0;
	inline int lastDelayTicks = 0;
	inline bool predictionAvailable = false;
	inline CCLabelBMFont* hud = nullptr;

	inline int windowFrames() { return kWindows[windowIdx]; }
	inline int windowTicks() { return windowFrames() * ticksPerFrame; }
	inline void clear() { queue.clear(); }
	inline bool on() { return Mod::get()->getSettingValue<bool>("enabled"); }
	inline bool active(GJBaseGameLayer* l) { return on() && l == PlayLayer::get(); }
}

// ---- input hook ------------------------------------------------------------
class $modify(BufGJBGL, GJBaseGameLayer) {
	// button 1 = jump. Left/right (platformer) are never touched.
	void handleButton(bool down, int button, bool isPlayer1) {
		if (!buf::active(this) || button != 1) {
			GJBaseGameLayer::handleButton(down, button, isPlayer1);
			return;
		}

		if (!down) {
			// RELEASE: cancels a queued press for this player, else immediate.
			for (auto it = buf::queue.begin(); it != buf::queue.end(); ++it) {
				if (it->p1 == isPlayer1) {
					buf::queue.erase(it);
					log::info("[PressBuffer] release cancelled queued press");
					return;
				}
			}
			GJBaseGameLayer::handleButton(false, button, isPlayer1);
			return;
		}

		// PRESS: decide delay in ticks (0 = immediate)
		int delay = 0;
		auto v = predictSurvival(this, isPlayer1, buf::windowTicks());
		buf::predictionAvailable = (v != Verdict::Unavailable);
		if (Mod::get()->getSettingValue<bool>("force-delay-debug")) {
			delay = buf::windowTicks();
		}
		// (Step 2: if v==Unsafe, search delays 1..window for first Safe)

		if (delay <= 0) {
			buf::lastDelayTicks = 0;
			GJBaseGameLayer::handleButton(true, button, isPlayer1);
		} else {
			buf::queue.push_back({isPlayer1, delay, delay});
			log::info("[PressBuffer] queued press for {} ticks", delay);
		}
	}

	// runs once per physics tick; applies queued presses when their time is up
	void processCommands(float dt) {
		GJBaseGameLayer::processCommands(dt);
		if (!buf::active(this)) return;
		buf::tickCount++;

		auto pl = PlayLayer::get();
		if (pl->m_player1 && pl->m_player1->m_isDead) { buf::clear(); return; }

		for (size_t i = 0; i < buf::queue.size();) {
			auto& p = buf::queue[i];
			if (--p.ticksLeft <= 0) {
				bool p1 = p.p1;
				buf::lastDelayTicks = p.total;
				buf::queue.erase(buf::queue.begin() + i);
				GJBaseGameLayer::handleButton(true, 1, p1);   // original, bypasses our hook
				log::info("[PressBuffer] applied delayed press");
			} else i++;
		}
	}
};

// ---- lifecycle clears + HUD ------------------------------------------------
class $modify(BufPlayLayer, PlayLayer) {
	void setupHasCompleted() {
		PlayLayer::setupHasCompleted();
		buf::clear();
		auto l = CCLabelBMFont::create("", "bigFont.fnt");
		l->setScale(0.3f);
		l->setAnchorPoint({0.f, 1.f});
		l->setAlignment(kCCTextAlignmentLeft);
		auto ws = CCDirector::get()->getWinSize();
		l->setPosition({6.f, ws.height - 6.f});
		l->setOpacity(160);
		m_uiLayer->addChild(l, 9999);
		buf::hud = l;
	}

	void update(float dt) {
		PlayLayer::update(dt);
		if (++buf::frameCount >= 60) {
			int t = (int)std::lround((double)buf::tickCount / buf::frameCount);
			buf::ticksPerFrame = std::clamp(t, 1, 16);
			buf::tickCount = 0; buf::frameCount = 0;
		}
		if (!buf::hud) return;
		if (!buf::on()) { buf::hud->setString(""); return; }
		float lastF = buf::ticksPerFrame ? (float)buf::lastDelayTicks / buf::ticksPerFrame : 0.f;
		buf::hud->setString(fmt::format(
			"Window: {}f ({} ticks, {} ticks/frame measured)\n"
			"Queued: {}\n"
			"Last delay: {:.2f}f ({} ticks)\n"
			"Prediction: {}",
			buf::windowFrames(), buf::windowTicks(), buf::ticksPerFrame,
			buf::queue.empty() ? "no" : "YES",
			lastF, buf::lastDelayTicks,
			buf::predictionAvailable ? "available" : "UNAVAILABLE (step 1: passing presses through)"
		).c_str());
	}

	void resetLevel() { buf::clear(); PlayLayer::resetLevel(); }
	void loadFromCheckpoint(CheckpointObject* cp) { buf::clear(); PlayLayer::loadFromCheckpoint(cp); }
	void togglePracticeMode(bool b) { buf::clear(); PlayLayer::togglePracticeMode(b); }
	void destroyPlayer(PlayerObject* p, GameObject* o) { buf::clear(); PlayLayer::destroyPlayer(p, o); }
	void onQuit() { buf::clear(); buf::hud = nullptr; PlayLayer::onQuit(); }
};

// ---- F6 cycles the window during gameplay ----------------------------------
class $modify(BufKeys, CCKeyboardDispatcher) {
	bool dispatchKeyboardMSG(enumKeyCodes key, bool down, bool repeat, double timestamp) {
		if (down && !repeat && key == KEY_F6 && PlayLayer::get() && buf::on()) {
			buf::windowIdx = (buf::windowIdx + 1) % 5;
			buf::clear();
			log::info("[PressBuffer] window = {} frames = {} ticks",
				buf::windowFrames(), buf::windowTicks());
		}
		return CCKeyboardDispatcher::dispatchKeyboardMSG(key, down, repeat, timestamp);
	}
};
