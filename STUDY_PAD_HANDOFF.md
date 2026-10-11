# Study Pad: handoff summary for a new chat

Read `CLAUDE.md` first. Follow it exactly: all logic in C++, Allman braces, lines of 70 characters or fewer, description comments, and constructor initialization. The "Git Workflow" section also applies.

## Project
- Unreal Engine 5.8 game, module `MemoryPalace`. The student writes homework on an iPad web page. A local Node server has Claude grade it. The game shows the live work on a whiteboard, awards XP, and unlocks sections.
- Branch: `feature/study-pad`, based on `dylan`. Both are pushed to origin, but the newest commits on `feature/study-pad` are not. Do not merge into `dylan` or `main`; Dylan does that after testing.
- Demo is due **3:30 PM** the day after the session start. It is about 8 minutes long.
- Dylan's brother is building the level's 9 buildings on another branch. Each building is one section of the course.

## Server (outside the repo): `C:\Users\O\Downloads\TestWebsite`
Files: `server.js`, `index.html` (iPad page), `problems.json`. Backups are saved as `*.bak-before-*`. The server runs at `http://127.0.0.1:3000`.

**Security:** an `ANTHROPIC_API_KEY` file and a `set` file in that folder contain a real API key that was pasted into the chat. Never read or print them. Tell Dylan to revoke and replace the key.

Endpoints:
- `GET /latest.png`: the whiteboard image (raw PNG, 404 until something is drawn).
- `GET /result.json`: review state. `?source=game` marks the poll as coming from the game.
- `POST /start?problem=<id>`: the game starts a problem.
- `GET /current.json`: the iPad polls it for the active problem. It carries a `startNumber` that increases on every start.
- `POST /submit`: returns 409 if there is no active problem.
- `POST /hint`: gives a hint that gets stronger on each press, up to 3. It never reveals answers.
- `POST /end`: the game closed. The server also ends the problem if the game stops polling for 5 seconds.
- `GET /curriculum.json`: built from `problems.json`.
- `GET /work?problem=<id>`: the saved board for a problem, as `{ revision, work: { documentItems, camera } }` (`work` is null if nothing is saved).
- `PUT /work?problem=<id>`: saves a board (`{ documentItems, camera }`) and returns the new `revision`. The server keeps the boards in `savedwork.json`, next to `server.js`.

Behavior:
- Problems are started only from the game. The iPad has no problem buttons.
- The iPad page draws the problem statement at the top of every exported PNG.
- The export is a fixed 1600 px wide and uses a fixed aspect ratio (default square, override with `?boardAspect=`). Work is shrunk to fit and never stretched.
- The server is the source of truth for each problem's board. The page loads the saved board when a problem starts, pushes its changes about once a second, and pulls newer revisions from other open pages. Last save wins, and pan and zoom stay per page. Because of this, every client (iPad, browser, the game's embedded page) shows the same board, and a new attempt continues from the saved board instead of a blank one.
- A fully correct review ends the problem.
- Wrong parts never get a `correctAnswer`, and the AI prompt forbids revealing answers.
- `problems.json` has `subjectName`, `sectionName`, `difficultyRating` and `statement` for each problem. The finished result carries `subjectName`, `sectionName` and `difficultyRating`.

## Unreal code (all under `Source/MemoryPalace/Public|Private`)
- `Game/StudyPadSubsystem`: a world subsystem and the only class that does HTTP.
  - Polling starts only through `StartPolling()`.
  - Methods: `StartProblem`, `RequestHint`, and `EndProblem` (called from `Deinitialize`).
  - Images are skipped by checksum. A review left over from before play does not fire. Each result fires once.
  - Events: `OnWorkImageUpdated`, `OnReviewStarted`, `OnReviewFinished`, `OnReviewFailed`, and `OnHintUpdated`.
- `Data/StudyReviewResult`: plain structs parsed from JSON. `Data/StudyTopicKind.h`: the Subject/Section enum.
- `Data/StudyCurriculumDataAsset`: a hand-made section list with unlock thresholds. **It is to be replaced by server data and deleted.**
- `Game/MemoryPalaceGameMode`: the rules.
  - XP per correct part is `ExperiencePerDifficultyPoint` (50) times the difficulty rating, divided by the number of parts.
  - A repeat of a part pays `MinimumRepeatFraction` (0.1) and recovers over `RepeatRecoverySeconds` (3600).
  - Each part is rewarded once per attempt.
  - Sections unlock when the prerequisite section reaches an XP threshold.
  - It exposes `StartProblem` and `RequestHint`, and sets `AStudyGameState` and `AStudyPlayerState`.
- `Game/StudyGameState` (unlocked sections, active problem) and `Player/StudyPlayerState` (XP per kind and topic, last award time per part).
- `Player/MemoryPalacePlayerController`: the `InteractAction` and `HintAction` inputs.
- `Interaction/StudyInteractable` (Interact starts a problem) and `Interaction/StudyPageable` (previous and next page, bound to the Left and Right arrow keys).
- `Actor/StudyWhiteboardActor`: one study station in one Blueprint, with one trigger box (the player must stand in it). It holds:
  - the whiteboard mesh and `Component/StudyWhiteboardComponent`, a dynamic material with texture parameter `WhiteboardTexture`. The component has the `ProblemId`. It can show the live image or a saved submission's PNG. It does **not** resize the mesh. Dylan wants the mesh size left alone.
  - `StudyPageWidgetComponent`: the real iPad page, shown in a `UStudyWhiteboardBrowserWidget` (web browser) only while the player holds Ctrl inside the trigger. A timer asks it to redraw, because a widget component otherwise redraws only while the cursor is over it.
  - `HistoryWidgetComponent`: a separate object showing the submission history through `UStudyGradeHistoryWidget`.
  - `Component/StudyHistoryComponent`: pages through the saved submissions (arrow keys or the on-widget arrows), shows each page's work on the whiteboard, and jumps to the newest submission when a review is saved. The "Start New Attempt" button on the widget and the Interact key both start the problem.
- `Component/StudyCursorInteractionComponent`: a widget interaction component on the player controller that follows the mouse. While Ctrl is held, the controller shows the cursor, locks the camera and movement, and forwards left click, wheel and typed keys (letters follow Shift and Caps Lock, and Shift symbols use a US layout).
- `Data/StudyServerAddress.h`: the server URL, shared by the subsystem and the browser widget.
- `Game/StudyProgressSubsystem`: saves each submission with a copy of the board PNG, and fires `OnSubmissionRecorded`.
- `UI/HUD/StudyHUD` owns `UI/Widget/StudyFeedbackWidget` and `StudyExperienceWidget`. The feedback panel shows "Reviewing...", review failures and hints. The review result itself is shown on the history widget, not on screen. The XP widget starts with empty bars.

## Decisions made
- XP is the unlock measure. The AI sets `difficultyRating`, and XP scales with it.
- No per-problem data assets. The AI decides the content.
- Partial credit counts. Wrong parts must be solved by the student. Hints are available.
- Visuals (parabola and similar) are skipped for now.
- Account login with Claude is not possible. Use an API key only. Plan: a `POST /config` endpoint on the server, built at the settings step.
- Dylan gave permission to change `server.js` and `index.html`.
- Use a stronger model, Opus 5.5, to double-check or write answer keys. Reading and grading uses `claude-sonnet-5-5`.

## Remaining plan, in order
1. **Curriculum parsing:** parse `/curriculum.json` in the subsystem. Switch the game mode's unlocking to use it, with each section's `unlockFraction` applied to the previous section's available XP. Delete `UStudyCurriculumDataAsset`.
2. **Buildings:** write `AStudyBuilding` (a marker actor with slot points). The game mode assigns sections to buildings and problems to slots, and locks or unlocks buildings from section state. A building Blueprint can be made, but Dylan can't place buildings until his brother's branch lands.
3. **Server AI pipeline (Tier 2, the wow factor):** an upload page for a PDF or photo plus a text box for what is assigned. `POST /materials` has Claude produce sections, problems, parts, answer keys and `difficultyRating`. Opus double-checks or writes the keys. Add an `expression` part kind for answers the four current kinds can't grade. Cache the last good curriculum as a stage fallback. The result should show generated problems for Dylan to review before presenting.
4. **Optional:** free rehearsal mode on the server (a fake correct result), XP cost for hints, and the title-screen and world-creation menus (cut for the demo).

## Editor setup that is not in code
- `WBP_StudyWhiteboardBrowser` (parent `StudyWhiteboardBrowserWidget`) needs a Web Browser widget named `StudyWebBrowser`. The `WebBrowserWidget` plugin must be enabled.
- `WBP_StudyGradeHistory` needs the text blocks `ProblemTitleTextBlock`, `PageTextBlock`, `ScoreTextBlock` and `DetailsTextBlock`, and the buttons `PreviousButton`, `NextButton` and `StartNewButton`.
- The whiteboard Blueprint sets the widget classes of `StudyPageWidgetComponent` and `HistoryWidgetComponent`, the `ProblemId` on `WhiteboardComponent`, and the size of `TriggerBox`.
- `BP_StudyHistoryBoard` and `BP_StudyProblemStation` are obsolete, since their classes were merged into the whiteboard actor. Delete them and their placed instances.

## Not yet verified
- The study station was built and tested by Dylan in the editor. Older steps were only compiled where noted. Dylan builds in the editor.
- The fully-correct path (ending the problem) and the AI's answer secrecy need real API calls to confirm.
- The `unreal-mcp` server failed to connect in the latest session (ECONNREFUSED). Earlier sessions had editor tools; give Dylan copy-and-paste prompts for editor work when the MCP is unavailable.

## Working style Dylan likes
- Small commits with imperative messages, ending with `Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>`.
- Run Rider's lint, and check the 70-character limit, before committing.
- Keep answers short and clear. When Dylan is wrong or something is risky, say so plainly.
- Ask before pushing.

---

# Update (session 2): per-board images, save/load, history board

Read `CLAUDE.md` first (it also has the Git Workflow). Branch `feature/study-pad`, nothing pushed. Ask before pushing. `unreal-mcp` may be down; give Dylan copy-and-paste prompts for editor work.

## Done and committed (compiled by Dylan's MCP, PIE wrote the save file)
- `4dcf5d7` Each whiteboard shows its own problem. `server.js` sends an `X-Problem-Id` header on `/latest.png` and clears the image on `/start`. `UStudyPadSubsystem` keeps one texture per problem. `OnWorkImageUpdated(ProblemId, Texture, Aspect)`, `GetTextureForProblem`. `UStudyWhiteboardComponent` has an editable `ProblemId`.
- `95dfb66` / `c401484` Save and load, one slot per world (`StudyProgress_<MapName>`): `Data/StudyProgressSaveGame` (boards, submissions, XP, part award times) and `Game/StudyProgressSubsystem`. Saves after each finished review, when switching problems, and on world teardown. The game mode restores XP in `PostLogin`. `FMath::RoundToInt` returns int64 here, so keep int64 locals (cast on return).
- `cb3f371` / `6cb3f78` First grade history widget (arrow buttons, HUD toggle). **Dylan did not want this.** He wants history on an object in the level.

## Web page and server (outside the repo, `C:\Users\O\Downloads\TestWebsite`)
- The iPad page saves work per problem and restores it when a problem is started again. The server now holds the real copy (`GET` and `PUT /work`, stored in `savedwork.json`), so every open page shares one board; `localStorage` is only a fallback and keeps the camera. It does not upload a blank frame when a problem ends.
- A Pause/Resume button. The server keeps the elapsed time per problem and a pause state (`POST /pause`, `POST /resume`; `/current.json` has `isPaused`).
- `/frame?problem=<id>` drops late frames from a problem that is no longer active.
- Backups: `*.bak-before-perboard`, `*.bak-before-pause`, `*.bak-before-serversync`. Server and page changes are not in git.
- Still true: never read or print `ANTHROPIC_API_KEY` or `set` in that folder; tell Dylan to revoke the key.

## Done: the study station (history, whiteboard and problem start in one actor)
Dylan tested it in the editor and it works. See the "Unreal code" section above for the classes. In short:
- History lives on a separate widget component of `AStudyWhiteboardActor`, paged with the arrow keys, the on-widget arrows (clicked with the Ctrl cursor) or a "Start New Attempt" button. Each page shows its work image on the whiteboard, and a newly saved submission appears without any input.
- Holding Ctrl inside the trigger box shows the full iPad page on the whiteboard (a web browser widget), driven by the mouse and keyboard.
- The HUD toggle and History key are gone. `AStudyHistoryBoard` and `AStudyProblemStation` were merged into the whiteboard actor.
- Gotchas: a trigger box set to `OverlapOnlyPawn` still blocks the Visibility channel and eats the cursor trace, so the actor ignores that channel in `BeginPlay`. `FInputModeGameAndUI` hides and freezes the cursor while a button is held unless `SetHideCursorDuringCapture(false)` is set. A widget component redraws only on request.
- Build note: Live Coding cannot apply header changes or new classes. Close the editor and build from Rider, or run `Build.bat MemoryPalaceEditor Win64 Development -Project=...` from `D:\Program Files\Epic Games\UE_5.8`.

## Remaining, in order
1. PDF export of solved problems in one file (a button in the castle; not on the castle branch yet). Plan: the server builds the PDF from the saved boards; the game calls an endpoint.
2. Send failed or API-error results to the history widget as well, and show the API error message for 5-10 seconds. Failed reviews are not recorded in the save yet.
3. Orb-style cursor particle effect (Niagara; editor work for Dylan).
4. Optional: a fresh blank board per attempt (a new attempt currently continues from the saved board), and gamepad support for the cursor and arrows.
5. From the first handoff: curriculum parsing from `/curriculum.json` and deleting `UStudyCurriculumDataAsset`, buildings, the server AI upload pipeline.

## Working style
Small commits with imperative subjects ending `Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>`. Lines of 70 characters or fewer (tab = 4). Short answers, say plainly when something is risky. Dylan likes prompts for the Unreal MCP for editor steps.
