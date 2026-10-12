# Memory Palace

A first-person study game built in **Unreal Engine 5.8**. You do real
homework by hand on a tablet, and the game turns it into progress: an
AI tutor (Claude) grades each answer, awards XP, gives hints without
giving away answers, and unlocks new sections of your palace, a ring of
castles on a tropical island, as you master the material.

> **Try it:** a Windows build is on the
> [Releases page](https://github.com/DylanJFernandes/Memory-Palace/releases).
> It needs the small local server in [`Server/`](Server) and your own
> Anthropic API key. See [Quick start](#quick-start).

---

## How it works

```
 ┌──────────────┐  draws on   ┌───────────────────────┐   grades with   ┌────────────┐
 │ Tablet/phone │ ──────────▶ │  Study server (Node)  │ ──────────────▶ │ Claude API │
 │  web page    │ ◀────────── │  http://<pc>:3000     │ ◀────────────── │            │
 └──────────────┘  problems   └───────────────────────┘    results      └────────────┘
                                   ▲             │
                    start problem, │             │ live whiteboard image,
                    hints, API key │             ▼ reviews, curriculum
                              ┌───────────────────────┐
                              │  Memory Palace (UE5)  │
                              └───────────────────────┘
```

1. **Create a world.** The game shows a QR code. Scan it on a tablet or
   phone and upload photos or PDFs of your homework. Claude turns them
   into sections and problems with answer keys and difficulty ratings.
2. **Walk the palace.** Each castle is a section of the course, and each
   whiteboard station inside it holds one problem.
3. **Solve on the tablet.** Step up to a whiteboard and press **E** to
   start. Your handwriting streams live onto the in-game whiteboard.
4. **Get graded.** Submit on the tablet. Claude reads each part, and
   correct parts award XP scaled by difficulty. Wrong parts get feedback
   and never the answer. Press **H** for a hint (up to 3 per problem,
   each a bit stronger).
5. **Unlock more.** Earning XP in one section unlocks the next. Every
   submission is saved, and you can page back through your history on
   the board.

## Features

- Live handwritten work mirrored onto in-world whiteboards
- AI grading of numbers, points, number sets and multiple choice, with
  partial credit
- Hints that get stronger on each press but never reveal answers
- Curriculum generated from your own uploaded homework (PDF or photos)
- XP progression and section unlocks, with diminishing XP for repeats
- Per-problem submission history, browsable in the world
- Hold **Ctrl** at a station to use the full tablet page in-game with
  mouse and keyboard
- Title screen, world select/create/delete, loading screen, pause menu
  and settings
- Procedurally generated castles and a custom island art kit

---

## Quick start

### Requirements

| Need | Notes |
|---|---|
| Windows 10/11, 64-bit | For the prebuilt game |
| [Node.js 18+](https://nodejs.org) | Runs the study server. No `npm install` is needed. |
| Anthropic API key | Create one at [console.anthropic.com](https://console.anthropic.com). Grading, hints and curriculum generation use it, and usage is billed to your account. |
| Tablet or phone (optional) | Any device with a browser on the **same Wi-Fi** as the PC. Without one, you can write on the board in-game with Ctrl + mouse. |

### 1. Start the server

```bash
git clone https://github.com/DylanJFernandes/Memory-Palace.git
cd Memory-Palace/Server
node server.js
```

The console prints the LAN address(es) the tablet can open, for example
`http://192.168.1.20:3000`. Keep this window open while playing.

You can give the server your key in either of two ways:

- **In the game (easiest):** Title screen → **Settings** → paste the
  key → Save. The game sends it to the server on every launch.
- **As an environment variable:**
  ```bash
  # Windows (cmd)
  set ANTHROPIC_API_KEY=your_key_here
  node server.js
  ```
  ```bash
  # macOS / Linux
  ANTHROPIC_API_KEY=your_key_here node server.js
  ```

Without a key, the server still runs. Reviews return a placeholder
result and hints ask you to set a key.

### 2. Run the game

Download `MemoryPalace-Windows.zip` from the
[latest release](https://github.com/DylanJFernandes/Memory-Palace/releases),
unzip it, and run `MemoryPalace.exe`. Then:

1. **Settings** → paste your API key → Save (unless you set the
   environment variable).
2. **Play** → **Create World** → scan the QR code with your tablet and
   upload homework, or continue to use the sample curriculum.
3. Walk up to a whiteboard and press **E** to start its problem.

> Windows Firewall may ask whether Node.js can use the network the
> first time the server runs. Allow it on **private networks** so the
> tablet can connect.

---

## Controls

| Action | Keyboard / mouse | Gamepad |
|---|---|---|
| Move | W A S D | Left stick |
| Look | Mouse | Right stick |
| Jump | Space | A / Cross |
| Start the problem at a whiteboard | E | — |
| Ask for a hint | H | — |
| Use the tablet page on the whiteboard | Hold Left Ctrl, then mouse + keyboard | — |
| Page through submission history | ← / → | — |
| Pause menu | Esc | Start / Menu |

---

## Connections and ports

| What | Address | Notes |
|---|---|---|
| Game → server | `http://127.0.0.1:3000` | Hardcoded in [`StudyServerAddress.h`](Source/MemoryPalace/Public/Data/StudyServerAddress.h). The server must run on the **same PC** as the game. |
| Tablet → server | `http://<pc-lan-ip>:3000` | Printed by the server at startup and shown as a QR code in the game. |
| Server → Claude | `https://api.anthropic.com` | Uses `claude-sonnet-5-5` for reading and grading, and `claude-opus-5-5` for curriculum generation. |
| Game → QR service | `https://api.qrserver.com` | Receives only the local upload URL, to draw the QR code. |

The server listens on port **3000** on all interfaces (`0.0.0.0`) with
no authentication. Only run it on a network you trust.

<details>
<summary>Server endpoints</summary>

| Method | Path | Used by | Purpose |
|---|---|---|---|
| GET | `/` | Tablet | The handwriting page |
| GET | `/upload` | Tablet | Homework upload page |
| POST | `/config` | Game | Sets the API key (`{ "apiKey": "…" }`) |
| GET | `/lan-url.json` | Game | Upload URL for the QR code |
| POST | `/materials` | Tablet | Uploads homework files |
| POST | `/generate` | Game | Builds problems from the upload |
| GET | `/materials/status` | Game | Generation progress |
| GET | `/curriculum.json` | Game | Sections and problems |
| POST | `/start?problem=<id>` | Game | Starts a problem |
| GET | `/current.json` | Tablet | Active problem |
| POST | `/frame?problem=<id>` | Tablet | Live board image |
| GET | `/latest.png` | Game | Latest board image |
| GET / PUT | `/work?problem=<id>` | Tablet | Saved board per problem |
| POST | `/submit` | Tablet | Submits work for grading |
| GET | `/result.json` | Game | Latest review |
| POST | `/hint` | Game | Next hint for the active problem |
| POST | `/pause`, `/resume` | Tablet | Pause the problem timer |
| POST | `/end` | Game | Ends the active problem |

</details>

### Curriculum format

[`Server/problems.json`](Server/problems.json) holds the problems the
server serves. The repo ships a two-problem sample (quadratic
functions). Uploading homework from the Create World screen replaces
this file with a generated curriculum. Each problem has an `id`, a
`sectionId`/`sectionName`, a `difficultyRating`, the `equation` or
`statement`, and `parts`. Each part is one of `number`, `point`,
`numberSet` or `choice`, with its correct answer. See the comment at
the top of [`server.js`](Server/server.js) for examples.

---

## Security and your API key

- **No key is stored in this repository.** The server reads it from
  `ANTHROPIC_API_KEY` or receives it from the game at runtime, and keeps
  it in memory only. [`Server/.gitignore`](Server/.gitignore) excludes
  key files and runtime data.
- The game saves the key you enter in Settings **in plain text** in its
  save file:
  - Packaged build: `MemoryPalace/Saved/SaveGames/StudyMenu.sav`,
    next to the `.exe`
  - Editor: `Saved/SaveGames/StudyMenu.sav` in the project

  **Don't share or zip your game folder after entering a key.** The
  release build ships without a `Saved` folder.
- Set a monthly spend limit on your Anthropic account if you share the
  game with others.

---

## Building from source

1. Install **Unreal Engine 5.8** from the Epic Games Launcher.
2. Install **Visual Studio 2022** (or Rider) with the components in
   [`.vsconfig`](.vsconfig). Opening the file in the Visual Studio
   Installer selects them for you.
3. Right-click `MemoryPalace.uproject` → **Generate Visual Studio
   project files**, then build the `MemoryPalaceEditor` target
   (Development Editor, Win64). Opening the `.uproject` and letting it
   rebuild also works.
4. Start the server (see above) and press **Play** in the editor. The
   default map is `Content/Maps/Lvl_TitleScreen`, and the study level is
   `Content/FirstPerson/Lvl_FirstPerson`.

The project uses the built-in **WebBrowserWidget** plugin for the
in-game tablet page. It is enabled in the `.uproject`.

To package, use **Platforms → Windows → Package Project**. The cooked
maps are already listed in `Config/DefaultGame.ini`.

### Project layout

```
Source/MemoryPalace/
  Public/ & Private/
    Actor/        Castles, castle ring, whiteboard study stations
    Character/    First-person character
    Component/    Problem slots, whiteboard, history, cursor interaction
    Data/         Curriculum, review results, save games, server address
    Game/         Game modes, game state, subsystems (server HTTP, progress, menus)
    Interaction/  StudyInteractable and StudyPageable interfaces
    Player/       Player controller, player state, camera manager
    UI/           HUDs (menu, loading, study) and widgets
Content/
  Blueprints/     Blueprint children of the C++ classes, widgets, input
  Castle/         Castle meshes, materials, FX
  Maps/           Title screen and loading levels
Server/           Node study server and tablet pages
Art/              Source art for the island kit (Blender + FBX)
Docs/             Development notes
```

Coding conventions for contributors are in [`CLAUDE.md`](CLAUDE.md).

---

## Credits

Made by **Dylan Fernandes** (gameplay, study systems, server) and
**Ethan** (castle level, procedural castles, island art).
AI grading and generation by [Claude](https://www.anthropic.com/claude).
