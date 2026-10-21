# Press Buffer, STEP 1 (unverified, never compiled)

**Honest status:** written without access to the Geode SDK, so it has NOT been compiled or run.
Expect possible compile errors. Copy the red error text from GitHub and send it back.

## What step 1 does
- Queues jump presses (button 1 only), applies them after N physics ticks.
- Release cancels a queued press; releases are never delayed.
- Clears the queue on death, restart, checkpoint load, practice toggle, level exit, window change.
- HUD (top-left): window, queued yes/no, last delay (frames + ticks), prediction status.
- F6 cycles window: 1, 2, 4, 8, 16 frames.
- **Prediction is NOT implemented.** Normally every press passes straight through (0 delay).
  To check the queue itself, turn on the setting "DEBUG: always delay every press".

## Units
- 1 frame = 1 displayed frame at 60 FPS = (TPS / 60) ticks. At 240 TPS: 1 frame = 4 ticks, 16 frames = 64 ticks.
- The ticks-per-frame number is measured every 60 frames and shown on the HUD.

## Build it (no coding needed)
1. Make a free account at github.com.
2. Click **+** (top right) > **New repository**. Name it `press-buffer`, keep it Public, click **Create repository**.
3. Click **uploading an existing file**. Unzip the project on your PC first, then drag in the **contents** of the folder (`src`, `mod.json`, `CMakeLists.txt`, `README.md`) and click **Commit changes**.
4. The workflow file is in a hidden folder `.github/workflows/build.yml`. Drag-and-drop often skips hidden folders. Instead: **Add file > Create new file**, type `.github/workflows/build.yml` as the name (typing the slashes makes folders), paste in the contents of that file, **Commit changes**.
5. Click the **Actions** tab. If asked, click "I understand my workflows, enable them". Click **Build Geode Mod** on the left, then **Run workflow** > **Run workflow**.
6. Wait about 5 to 10 minutes. A green check = success, a red X = failure. If red, click the run, click the red job, and copy the red error lines to me.
7. On a green run, scroll to **Artifacts** at the bottom of the run page and download the zip. Unzip it to get `anthony.press-buffer.geode`.
8. Put that file in `C:\Users\<you>\AppData\Local\GeometryDash\geode\mods` (or open GD > Geode button > Mods > the folder icon). Restart GD.

## Test plan (step 1)
1. Turn on the DEBUG delay setting. Press jump in a level: HUD should show "Last delay" = window size (4f = 16 ticks) and the jump should visibly lag.
2. Press and release quickly: the queued jump should NOT happen (cancelled). Check the Geode console for "release cancelled".
3. Hold jump: delay is applied once, then holds normally.
4. Press F6 five times: window goes 4, 8, 16, 1, 2, 4. HUD ticks should match (frames x measured ticks/frame).
5. Practice mode: place a checkpoint, die, respawn, press: still works. Toggle practice on/off mid-level: queue clears.
6. Turn DEBUG off: presses should pass straight through, last delay 0, "Prediction: UNAVAILABLE".
7. Try cube, ship, ball, UFO, wave, robot, spider, swing: confirm presses delay in debug mode in each.

## Not done / unverified
- Prediction (steps 2 and 3). Dual mode, moving objects, triggers: not handled.
- Hook signatures guessed from memory, most likely compile problems: `CCKeyboardDispatcher::dispatchKeyboardMSG` (timestamp argument), `GJBaseGameLayer::processCommands`, `PlayLayer::destroyPlayer`.
- Assumes processCommands runs once per physics tick, and that handleButton is where queued presses get applied. Check in test 1.
- Workflow: `sdk: v5.11.2` tag name and unpinned bindings are unconfirmed.
