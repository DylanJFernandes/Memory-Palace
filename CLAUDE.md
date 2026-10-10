# Unreal Engine C++ Coding Conventions

These rules apply to all code written or modified in this project.
Follow them exactly. When existing code breaks a rule, match these
conventions in any code you touch.

Project context: single-player game, ~30-hour scope. Favor simple,
direct solutions over long-term scalability work.

---

## 1. Language and Engine Usage

- All gameplay logic is written in C++. Do not put logic in
  Blueprints unless no C++ substitute exists (this is almost never
  the case).
- Blueprints are used only for class instancing, setting exposed
  default values, and widget layout.
- Do not add Blueprint-facing macros (`BlueprintCallable`,
  `BlueprintReadWrite`, `BlueprintImplementableEvent`, etc.)
  unless explicitly asked.

---

## 2. Folder Structure

Mirror Stephen Ulibarri's (Druid Mechanics) course layout.

### Source
Headers live in `Public/`, implementations in `Private/`. Both
trees use identical subfolders. Create a subfolder only when a
class needs it.

```
Source/<ProjectName>/
	Public/
		Actor/          Placed/spawned actors (pickups, props)
		Character/      Characters and character base classes
		Component/      UActorComponent / USceneComponent types
		Data/           UDataAsset and data structs
		Game/           GameMode, GameState, GameInstance
		Input/          Input configs and input components
		Interaction/    Interfaces (I-prefixed)
		Player/         PlayerController, PlayerState
		UI/
			HUD/        AHUD subclass
			Widget/     UUserWidget subclasses
	Private/            Same subfolders, .cpp files
```

Includes use the path relative to `Public/`:

```cpp
#include "Character/ParasiteCharacter.h"
#include "UI/Widget/GaugeWidget.h"
```

### Content
Blueprint subfolders mirror the Source subfolders so a
Blueprint sits in the same category as its C++ parent.

```
Content/
	Assets/             Imported art/audio, grouped by feature
	Blueprints/
		Actor/
		Character/
		Component/
		Data/
		Game/
		Input/
		Player/
		UI/
	Maps/
```

Asset names follow the standard prefixes (`BP_`, `WBP_`, `DA_`,
`SM_`, `SK_`, `M_`, `MI_`, `T_`, `IA_`, `IMC_`, etc.).

---

## 3. Class Responsibilities

Put code in the class that owns that responsibility. Do not
place logic in a class just because it is convenient.

Pick the class whose **lifetime** matches the data. Data stored
in a class with a shorter lifetime than it needs will be lost
(e.g. stats kept on a Pawn vanish when it dies).

| Class | Lifetime | Responsible for |
|---|---|---|
| `UGameInstance` | Application (survives map changes) | Global persistent state, session data, UI transitions between levels, carrying data across levels. Extend with `UGameInstanceSubsystem`. |
| `ULocalPlayer` | Application | Local input mappings, viewport, audio listener. Extend with `ULocalPlayerSubsystem` (e.g. Enhanced Input). |
| `UWorld` | Level | Active level simulation: line traces, timers, actor spawning. Extend with `UWorldSubsystem`. |
| `AGameModeBase` | Level | Game rules, win/lose conditions, spawning, default framework classes. |
| `AGameStateBase` | Level | Global game state (game phase, global timers, totals). |
| `APlayerController` | Level | The 3 Cs (Character, Camera, Controls), input processing, holds a reference to the HUD. |
| `APlayerState` | Level (survives Pawn respawns) | Per-player persistent stats (name, score, progression). |
| `APawn` / `ACharacter` | Shortest (destroyed on death/respawn) | Physical representation in the level. `ACharacter` adds movement component, capsule, skeletal mesh. Behavior delegated to components. |
| `AHUD` | Level | Local visual overlay manager; creates, owns, and initializes UMG widgets. |
| `UActorComponent` | Owner's lifetime | Reusable, interchangeable chunks of behavior. |
| `UUserWidget` | Owned by the HUD | Display only; reacts to delegates, no game logic. |
| `UDataAsset` | Asset | Designer-tunable data shared between instances. |

The game is single-player, so replication domains don't drive
placement, but keep the separation anyway: rules in GameMode,
shared state in GameState, per-player data in PlayerState.

### Reflection notes
- `UCLASS` objects are zero-initialized by the engine. Each has a
  Class Default Object (CDO) holding its default values, built
  from the constructor. This is why defaults belong in the
  constructor.
- Prefer a Subsystem over a manager actor or singleton when
  something needs global, lifetime-scoped access.

---

## 4. Architecture

- **Prefer composition over inheritance.** Build behavior as
  `UActorComponent` / `USceneComponent` subclasses that can be
  attached to any actor, instead of deep class hierarchies.
- Keep inheritance shallow: a base class plus direct children is
  usually enough.
- Use interfaces (`Interaction/`) for cross-class communication
  instead of casting to concrete types where practical.
- Modules and plugins are the same idea at a larger scale. Given
  the project's scope, keep everything in the main game module
  unless asked otherwise.

---

## 5. Naming

- Follow Epic Games' Unreal naming conventions. Use PascalCase
  for all types, functions, and variables.
- Use the standard Unreal prefixes: `A` (Actors), `U` (UObjects
  and components), `F` (structs and delegates), `E` (enums),
  `I` (interfaces), `T` (templates), `S` (Slate widgets),
  `b` (booleans).
- `#include "FileName.generated.h"` is always the **last**
  include in a header.
- Never shorten or abbreviate names. Names must fully describe
  what they hold or do.

```cpp
APlayerController* PC;                 // BAD
APlayerController* PlayerController;   // GOOD

for (int32 I{0}; I < Num; ++I)         // BAD
for (int32 RowIndex{0}; RowIndex < RowCount; ++RowIndex)  // GOOD
```

- Function parameters that initialize a member use the `In`
  prefix (`InStartingChance`).

---

## 6. Formatting

### Braces
All function, class, struct, and control-flow braces go on their
own line (Allman style).

```cpp
void Foo()
{

}
```

### No single-line control flow
Every `if`, `else`, `for`, and `while` body uses braces on their
own lines, even for one statement.

```cpp
if (bIsDead) Respawn();   // BAD

if (bIsDead)              // GOOD
{
	Respawn();
}
```

### Line length
- No line may exceed **70 characters** (count a tab as 4).
- When a line would exceed 70, break it and align continuation
  lines with the first argument or indent one extra level.

```cpp
void Init(float InStartingChance, float InTapIncrement,
		  float InEnemiesPerSecond,
		  const FGameplayTag& ParasiteRarityTag,
		  const FGameplayTag& AnimalRarityTag);

TWeakObjectPtr<UParasiteAbilitySystemComponent>
	CachedParasiteAbilitySystemComponent;
```

---

## 7. Clean Code Principles

Follow the practices from *Clean Code* (Robert C. Martin).

- **Single responsibility:** each function does one thing.
- **Function size:** aim for 5–20 lines. When a function grows
  past that, extract the extra work into a well-named helper.
- **One level of abstraction per function.** High-level
  functions call well-named helpers; they do not mix in
  low-level details.
- **Minimal repetition (DRY):** extract repeated logic into a
  shared function rather than duplicating it.
- **Few parameters:** prefer 0–3. If more are needed, consider a
  struct.
- **No side effects hidden behind misleading names.**
- **Early returns** over deep nesting.
- **Group related code together** within the same scope (class,
  function, etc.).
- **Declare local variables right before their first use**, not
  at the top of the function.

```cpp
void AParasiteCharacter::CachePlayerReferences()
{
	CachedPlayerState = GetPlayerState<AParasitePlayerState>();

	APlayerController* PlayerController{
		Cast<APlayerController>(GetController())};
	if (!PlayerController)
	{
		return;
	}
	CachedPlayerHUD = PlayerController->GetHUD<APlayerHUD>();
}
```

---

## 8. Access Levels

- A function is `public` **only** if another class calls it.
- Helper functions used only inside the class are `private`.
- `protected` is for engine overrides (`BeginPlay`, `Tick`,
  `SetupPlayerInputComponent`, etc.) and members child classes
  actually use.
- Start every new function as `private` and widen access only
  when a caller outside the class needs it.

---

## 9. File Ordering (Readability Top-Down)

Files read top to bottom like a story: lifecycle first, then the
functions it calls, then everything else.

### Header
Access sections appear in this order:

```cpp
public:
protected:
private:
```

Within each section:
1. **Functions first, variables last.**
2. Lifecycle functions first (constructor, `BeginPlay`, `Tick`),
   then the rest in the order they are called.
3. Related functions are grouped together; related variables are
   grouped together, with an optional `// Group Name` label.

Because access sections split the class, the header will not be
perfectly top-down; keep it as close as the sections allow.

### Source (.cpp)
Since the .cpp has no access sections, follow the call order
strictly (the *Clean Code* "stepdown rule"):

1. Constructor
2. `BeginPlay` (and other engine lifecycle overrides)
3. `Tick` (only if enabled; see Section 11)
4. Helpers, each placed directly below the function that first
   calls it
5. Remaining functions (interface implementations, delegate
   handlers, public API) in a logical reading order

### Interface functions
Overridden interface functions are wrapped in begin/end comments:

```cpp
	// Player Interface
	/* Returns whether a parasite inhabits this character */
	virtual bool GetIsInhabited() const override;

	/* Returns the XP multiplier applied to this character */
	virtual float GetXPMultiplierAmount() override;
	//~End of Player Interface
```

---

## 10. Comments

### Description comments (required)
Every class, function, and variable declaration has a description
comment directly above it, used for IDE hover tooltips:

```cpp
/* Speed applied while the character is sprinting */
float SprintSpeed;
```

### Explanatory comments (avoid)
Do not add inline comments to explain what code does. If code
needs a comment to be understood, refactor it (rename, extract a
function) until it reads clearly. Only comment when the reason
truly cannot be expressed in code (e.g. an engine quirk).

### Section comments
Group labels such as `// Cached Vars` are allowed to organize
related declarations.

---

## 11. Tick

- Tick is **disabled by default**. Set it off in the constructor:

```cpp
// Actors
PrimaryActorTick.bCanEverTick = false;

// Components
PrimaryComponentTick.bCanEverTick = false;
```

- If tick is disabled, **do not override `Tick` /
  `TickComponent`** at all.
- Prefer event-driven logic (delegates, timers via
  `FTimerManager`) over per-frame polling. Enable tick only when
  something genuinely must update every frame.

---

## 12. Variables and Initialization

- **Never initialize member variables in the header**, even when
  the type has a default constructor. All member values are set
  in the class constructor.
- **Initialize as early as the value allows:**
  1. Constructor: default values, `CreateDefaultSubobject`
     components, tick settings. Prefer this whenever possible.
  2. `BeginPlay`: anything depending on other runtime objects
     (controllers, player state, HUD, other actors).
  3. Later only when the dependency truly isn't ready until then.
- Prefer **brace initialization** everywhere a value is set.

```cpp
// Header
/* Number of enemies spawned per second */
float EnemiesPerSecond;

// Source
AEnemySpawner::AEnemySpawner()
	: EnemiesPerSecond{1.0f}
{
	PrimaryActorTick.bCanEverTick = false;
}

// Local variables
int32 SpawnCount{0};
FVector SpawnLocation{GetActorLocation()};
```

---

## 13. Pointers and Asset References

| Use | When |
|---|---|
| `TWeakObjectPtr<>` | Non-owning references to objects (most common; cached refs) |
| `TObjectPtr<>` | Owned references (components, widgets the class created) |
| `TSubclassOf<>` | A class to spawn/create from (widget classes, actor classes) |
| `TSoftObjectPtr<>` | Assets that are large or not always needed; load on demand |
| `TSoftClassPtr<>` | Classes that should not be loaded until needed |
| Raw pointer | Function parameters and short-lived locals only |

- Hard references (`TObjectPtr`, `TSubclassOf`) load the asset
  with whatever references it. Use soft pointers for heavy or
  rarely used assets (large meshes, sounds, level-specific data).
- Load soft references synchronously with `LoadSynchronous()`
  for small assets, or asynchronously via `FStreamableManager`
  when a hitch would be noticeable.

```cpp
	// Cached Vars
	/* Player state that owns this character */
	TWeakObjectPtr<AParasitePlayerState> CachedPlayerState;

	/* Death sound, loaded only when the character dies */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Audio")
	TSoftObjectPtr<USoundBase> DeathSound;
```

---

## 14. UPROPERTY and UFUNCTION

- Add `UPROPERTY()` **only** when a variable:
  - must be visible or editable in the editor, **or**
  - is an owning `TObjectPtr<>` to a UObject (required so the
    garbage collector tracks it and does not destroy it), **or**
  - uses `BindWidget` (see Section 15).
- Editor exposure uses one of:
  - `UPROPERTY(EditAnywhere, ...)`
  - `UPROPERTY(EditDefaultsOnly, ...)`
  - `UPROPERTY(VisibleAnywhere, ...)`
- Every editor-visible property must have a category of
  `"Custom"` or `"Custom|SubCategory"`.

```cpp
	/* Speed applied while the character is sprinting */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Movement")
	float SprintSpeed;

	/* Mesh component rendering the weapon */
	UPROPERTY(VisibleAnywhere, Category = "Custom|Components")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;
```

- Variables that need none of the above get **no**
  `UPROPERTY()`.
- Add `UFUNCTION()` **only** when a function is bound to an
  engine dynamic delegate with `AddDynamic` (e.g.
  `OnComponentBeginOverlap`). Otherwise, no `UFUNCTION()`.

---

## 15. UI and Widgets

### Ownership
- The **HUD** (`AHUD` subclass) owns all widgets. It holds each
  widget class (`TSubclassOf`), creates the widget in its own
  `BeginPlay`, keeps a `TObjectPtr` to it, and initializes it.
- The **PlayerController** holds a `TWeakObjectPtr` to the HUD
  (the controller spawns the HUD, but the HUD is a separate
  actor). Other classes reach widgets through the controller's
  HUD reference, never by holding widget pointers themselves.
- The HUD may not exist yet when the PlayerController's
  `BeginPlay` runs, so the controller caches the HUD lazily in a
  getter instead of in `BeginPlay`.
- Widgets are display only. Game logic stays in the owning
  gameplay classes; widgets update in response to delegates.

```cpp
// HUD header
protected:
	/* Creates and initializes all widgets */
	virtual void BeginPlay() override;

private:
	/* Creates the gauge widget and adds it to the viewport */
	void CreateGaugeWidget();

	/* Widget class used to create the gauge display */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|UI")
	TSubclassOf<UGaugeWidget> GaugeWidgetClass;

	/* Gauge widget created and owned by this HUD */
	UPROPERTY()
	TObjectPtr<UGaugeWidget> GaugeWidget;

// HUD source
void APlayerHUD::BeginPlay()
{
	Super::BeginPlay();
	CreateGaugeWidget();
}

void APlayerHUD::CreateGaugeWidget()
{
	GaugeWidget = CreateWidget<UGaugeWidget>(
		GetOwningPlayerController(), GaugeWidgetClass);
	GaugeWidget->AddToViewport();
}

// PlayerController header
public:
	/* Returns this controller's HUD, caching it on first use */
	APlayerHUD* GetPlayerHUD();

private:
	/* HUD spawned for this controller */
	TWeakObjectPtr<APlayerHUD> CachedPlayerHUD;

// PlayerController source
APlayerHUD* ATBPlayerController::GetPlayerHUD()
{
	if (!CachedPlayerHUD.IsValid())
	{
		CachedPlayerHUD = GetHUD<APlayerHUD>();
	}
	return CachedPlayerHUD.Get();
}
```

### BindWidget
- Use `meta = (BindWidget)` **only** for widget elements the
  C++ code reads or changes at runtime.
- Elements that are purely visual or never touched by code
  (static text, decorative images, layout panels) get **no**
  C++ variable. Set them up in the Widget Blueprint designer.

```cpp
	/* Progress bar showing the current gauge value */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> GaugeProgressBar;
```

- The same rule applies to any object: only declare a C++
  variable for something code actually needs to access.

---

## 16. Delegates

- Declare the simplest delegate type that works. Do not use
  `DYNAMIC` or `MULTICAST` variants unless they are actually
  required (they rarely are).

```cpp
DECLARE_DELEGATE_OneParam(FGaugeChanged, float);
```

- Bind non-dynamic delegates with `BindUObject` / `AddUObject`
  so no `UFUNCTION()` is needed.

---

## 17. Full Example

### Header
```cpp
/* Broadcast when the parasite gauge value changes */
DECLARE_DELEGATE_OneParam(FGaugeChanged, float);

/* Character that can be inhabited by a parasite player */
UCLASS()
class AParasiteCharacter : public ACharacter,
						   public IPlayerInterface
{
	GENERATED_BODY()

public:
	/* Sets default values for this character */
	AParasiteCharacter();

	// Player Interface
	/* Returns whether a parasite inhabits this character */
	virtual bool GetIsInhabited() const override;
	//~End of Player Interface

	/* Fires whenever the gauge value changes */
	FGaugeChanged OnGaugeChanged;

protected:
	/* Caches references when play begins */
	virtual void BeginPlay() override;

	/* Speed applied while the character is sprinting */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Movement")
	float SprintSpeed;

private:
	/* Caches the owning player state and HUD */
	void CachePlayerReferences();

	/* Updates the HUD with the new gauge value */
	void HandleGaugeChanged(float NewGaugeValue);

	/* Whether a parasite currently inhabits this character */
	bool bIsInhabited;

	// Cached Vars
	/* Player state that owns this character */
	TWeakObjectPtr<AParasitePlayerState> CachedPlayerState;

	/* HUD for the possessing player */
	TWeakObjectPtr<APlayerHUD> CachedPlayerHUD;
};
```

### Source
```cpp
AParasiteCharacter::AParasiteCharacter()
	: SprintSpeed{600.0f}
	, bIsInhabited{false}
{
	PrimaryActorTick.bCanEverTick = false;
}

void AParasiteCharacter::BeginPlay()
{
	Super::BeginPlay();
	CachePlayerReferences();
	OnGaugeChanged.BindUObject(
		this, &AParasiteCharacter::HandleGaugeChanged);
}

void AParasiteCharacter::CachePlayerReferences()
{
	CachedPlayerState = GetPlayerState<AParasitePlayerState>();

	APlayerController* PlayerController{
		Cast<APlayerController>(GetController())};
	if (!PlayerController)
	{
		return;
	}
	CachedPlayerHUD = PlayerController->GetHUD<APlayerHUD>();
}

void AParasiteCharacter::HandleGaugeChanged(float NewGaugeValue)
{
	if (!CachedPlayerHUD.IsValid())
	{
		return;
	}
	CachedPlayerHUD->UpdateGauge(NewGaugeValue);
}

bool AParasiteCharacter::GetIsInhabited() const
{
	return bIsInhabited;
}
```

---

## 18. Checklist Before Finishing Any Change

- [ ] Files placed in the matching `Public/` / `Private/` and
      `Content/Blueprints/` subfolders
- [ ] Code lives in the class responsible for it (Section 3)
- [ ] Components used instead of inheritance where practical
- [ ] No abbreviated names
- [ ] All braces on their own lines; no single-line `if`/`else`
- [ ] No line over 70 characters
- [ ] Every function is ~5–20 lines and does one thing
- [ ] No duplicated logic
- [ ] Local variables declared right before first use
- [ ] Helpers are `private`; `public` only when called externally
- [ ] Files read top-down: lifecycle, then helpers in call order
- [ ] Every class, function, and variable has a `/* */` comment
- [ ] No explanatory inline comments that a refactor could remove
- [ ] Header order: public → protected → private; functions
      above variables; related items grouped
- [ ] Interface overrides wrapped in `// X Interface` /
      `//~End of X Interface`
- [ ] Tick disabled in constructor; no `Tick` override if unused
- [ ] No member values set in the header; set as early as
      possible (constructor preferred, else `BeginPlay`)
- [ ] Brace initialization used
- [ ] Correct pointer type (weak / object / subclass / soft)
- [ ] `UPROPERTY`/`UFUNCTION` only where required, with
      `"Custom"` categories
- [ ] Widgets created and owned by the HUD; PlayerController
      holds a weak HUD reference; `BindWidget` only for
      elements code touches
- [ ] Data stored in a class whose lifetime outlasts its use
- [ ] `.generated.h` is the last include in every header
- [ ] Delegates use the simplest non-dynamic type possible
- [ ] No Blueprint logic or Blueprint macros

---

## 19. Additional Clean Code Smells

A condensed pull from *Clean Code*'s "Smells and Heuristics"
chapter. Watch for these while writing or reviewing any change.

### Comments
- **Don't let a comment hold information that belongs elsewhere**
  (an issue tracker, source control). No author/date/change-log
  comments; source control already has that history.
- **Delete obsolete and redundant comments** rather than letting
  them rot next to code that has since changed.
- A comment worth writing is worth writing well: correct grammar,
  no rambling, no stating the obvious.
- **Never leave commented-out code.** Delete it; source control
  remembers it if it's ever needed again.

### Functions
- **Too many arguments** is a smell on its own — prefer 0–3, and
  group related arguments into a struct if more are needed.
- **No output arguments.** A function should change the state of
  the object it's called on, not change a parameter passed to it
  by reference.
- **No flag (boolean) arguments.** A bool parameter means the
  function does more than one thing depending on its value — split
  it into two functions instead.
- **Delete dead functions.** A function nothing calls should be
  removed, not kept "just in case."

### General
- **One language per file.** Don't mix C++, Blueprint script
  fragments, and heavy markup in the same source file.
- **Implement the behavior a reasonable reader would expect**
  (principle of least surprise) — don't leave an obvious case
  unhandled.
- **Don't override a safety check** (disabling a warning, widening
  an access level) to silence a problem instead of fixing it.
- **No duplication.** The same logic in two places is a sign one
  of them should call the other, or both should call a shared
  helper.
- **Keep code at one level of abstraction per function.** Don't mix
  a high-level step with a low-level detail in the same function.
- **A base class should never depend on its derived classes.** If
  it needs to create them, use a factory instead of referencing
  the subclass directly.
- **A class, module, or function shouldn't expose more than
  callers need.** Keep internals `private`.
- **Prefer explaining variables** over a cryptic expression, so the
  reader doesn't have to decode the math or logic inline.
- **A function name should say exactly what the function does** —
  rename it if the name undersells or oversells its behavior.
- **Replace magic numbers with named constants.**
- **Negative conditionals are harder to read** than positive ones
  — prefer `if (bIsValid)` over `if (!bIsNotValid)`.
- **Prefer polymorphism to a chain of `if`/`else` or `switch` on
  type** when the same switch logic would otherwise be repeated in
  multiple places.

### Names
- **Choose names at the right level of abstraction.** A name
  should reflect the problem domain, not an implementation detail,
  unless the implementation detail is the point.
- **Use long, descriptive names for variables with a long scope**
  (member variables) and it's fine to use a shorter — but still
  real-word, never single-letter — name for a variable scoped to a
  few lines.

### Tests
- **Don't skip trivial tests**; they document behavior cheaply.
- **Test boundary conditions** carefully — the start, the end, and
  one past each.
- **Bugs tend to cluster.** When testing a function, test
  thoroughly around any area where a bug was just found.

---

## 20. Clean Code's Core Formatting Rule

> *The Newspaper Metaphor*: a source file should read like a news
> article — the name tells you what it is, the topmost content
> gives a high-level summary, and detail increases as you read
> down. This is why Section 9 orders both the header and the .cpp
> top-down by call order.

Keep files short enough to read without a lot of scrolling.
Related concepts stay close together vertically; unrelated ones
get a blank line between them.

---
---

# Git Workflow

Two authors — Dylan and Ethan — both have full authority over
this repository. There is no pull-request process and no
designated reviewer besides whichever of you is doing the merge:
you each review your own work before it lands on a branch the
other person builds on. A lot of commits will be made by Claude
Code acting on your behalf, so these rules are written to be
followed literally and mechanically, not just as a style guide.

## 1. Branch Structure

Three kinds of branch, in order of stability:

```
main                   Always buildable, always playable.
├── dylan              Dylan's long-running personal branch.
└── ethan              Ethan's long-running personal branch.
    ├── feature/<name> Short-lived, one feature or fix each.
```

- **`main`** — the shared, stable branch. Only receives finished,
  self-tested work merged in from a personal branch. Never commit
  to `main` directly and never merge a known-broken build into it.
- **`dylan`** / **`ethan`** — one long-running branch per person.
  This is where each of you does ongoing work and where your
  feature branches are born from and merged back into. Push this
  to the remote so the other person can see your in-progress work
  if needed, but treat it as yours: the other person doesn't
  commit to it.
- **`feature/<short-description>`** — created off your own
  personal branch for a single feature, fix, or piece of work
  (e.g. `feature/gauge-ui`, `feature/enemy-spawner`). Commit to it
  frequently and descriptively. It lives only as long as the
  feature takes, then gets merged and deleted.

This mirrors the standard Git "topic branch" workflow (Pro Git,
ch. 3): `main` is the always-stable branch, personal branches are
where work accumulates, and feature branches are the cheap,
disposable, short-lived branches Git is built for. Branches in Git
are just 41-byte pointers to a commit — creating and deleting them
is instant, so there's no cost to making one per feature.

## 2. Day-to-Day Flow

1. **Start a feature.** From your personal branch:
   ```
   git checkout dylan
   git pull origin dylan
   git checkout -b feature/gauge-ui
   ```
2. **Work and commit often.** Small, frequent, descriptive
   commits on the feature branch (see Section 4 for message
   style). It's fine for this history to be messy — it gets
   cleaned up at merge time if needed.
3. **Finish the feature.** Build it, test it in the editor, make
   sure it actually works before merging anything.
4. **Merge back into your personal branch:**
   ```
   git checkout dylan
   git merge feature/gauge-ui
   git branch -d feature/gauge-ui
   ```
   This fast-forwards if nothing else changed on `dylan` in the
   meantime, or creates a normal merge commit if it did. Either is
   fine for a personal branch.
5. **Keep going on your personal branch**, or, when you want your
   work combined with your brother's so you can test both
   features together, **merge your personal branch into `main`:**
   ```
   git checkout main
   git pull origin main
   git merge --no-ff dylan
   git push origin main
   ```
   `--no-ff` forces a real merge commit even when a fast-forward
   is possible, so `main`'s history shows exactly when each
   person's work was integrated — useful with two long-running
   branches feeding one trunk.
6. **Stay in sync.** Periodically merge `main` back into your own
   personal branch so it doesn't drift too far and conflicts don't
   pile up:
   ```
   git checkout dylan
   git merge main
   ```

## 3. Rebasing — Use Sparingly, and Only Locally

Pro Git's rule, stated plainly: **never rebase a branch that
exists outside your own machine** (i.e., anything already pushed
and that the other person might have pulled or built on). Rebasing
rewrites commits into new ones with new hashes; if someone else
already has the old ones, you get duplicated, confusing history
and a `git push --force` becomes necessary — which can overwrite
the other person's work.

- **Safe to rebase:** a `feature/*` branch that only you have been
  committing to and haven't pushed, to clean up messy or
  Claude-generated commits before merging it into your personal
  branch (`git rebase -i dylan` from the feature branch, or just
  squash with `git merge --squash` if you want one clean commit).
- **Never rebase:** `main`, or either personal branch, once it has
  been pushed. Always merge into these, never rebase them.
- If in doubt, merge. A few extra merge commits are harmless; a
  rewritten shared history is not.

## 4. Commit Messages

- **Imperative mood, present tense:** "Add gauge widget," not
  "Added gauge widget" or "Adding gauge widget."
- **Short subject line** (≈50 characters), blank line, then an
  optional body explaining *why* the change was made if it isn't
  obvious from the subject — not a restatement of the diff.
- **One logical change per commit.** Several small commits on a
  feature branch are preferred over one giant commit; they get
  squashed or left as-is at merge time either way.
- **Commits made by Claude Code** follow this exact convention
  too — no "WIP," no "fix," no vague messages. Each commit subject
  should be specific enough that `git log --oneline` on a feature
  branch reads like a changelog of what was built.

Example history on a feature branch:
```
Add FGaugeChanged delegate to AParasiteCharacter
Create UGaugeWidget bound to the gauge progress bar
Wire PlayerHUD to create and own the gauge widget
Fix gauge not clamping below zero
```

## 5. Useful Commands for This Workflow

| Command | Purpose |
|---|---|
| `git branch -vv` | See every local branch, what it tracks, and whether it's ahead/behind |
| `git branch --merged` | List branches already merged into the one you're on (safe to delete) |
| `git log --oneline --decorate --graph --all` | Visualize all branches and where they diverged |
| `git push origin <branch>` | Publish a branch so the other person can see it |
| `git push origin --delete <branch>` | Remove a finished branch from the remote |
| `git fetch --all` | Update your view of both remotes' branches without merging anything |

## 6. Unreal-Specific Git Hygiene

- **`.gitignore`** the generated/derived folders so they never get
  committed: `Binaries/`, `Intermediate/`, `Saved/`,
  `DerivedDataCache/`, `.vs/`, `*.VC.db`, `*.VC.opendb`.
- **Commit `.uproject`, `Config/`, `Source/`, and `Content/`**
  (the actual assets and code).
- Consider **Git LFS** for binary Unreal assets (`*.uasset`,
  `*.umap`) once the repo starts holding meshes, textures, or
  audio of real size — binary diffs bloat a normal Git repo fast.
  Not required to start, but worth setting up before content gets
  heavy.
- Before merging a personal branch into `main`, open the project
  and confirm it **compiles and the relevant map loads** — `main`
  staying buildable is the whole point of keeping it separate from
  the branches where work-in-progress lives.

## 7. Checklist Before Merging Into `main`

- [ ] Feature branch's work is merged into your personal branch
      and the feature branch is deleted
- [ ] Project compiles with no errors
- [ ] You've opened the editor and the change actually works
- [ ] Commit messages are descriptive (Section 4)
- [ ] `main` is pulled/up to date locally before you merge into it
- [ ] Merge into `main` uses `--no-ff`
- [ ] Pushed to `origin` so your brother can pull it
