// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetInteractionComponent.h"
#include "StudyCursorInteractionComponent.generated.h"

/* Lets the mouse cursor click, scroll and type into widgets that
   are drawn in the world */
UCLASS()
class MEMORYPALACE_API UStudyCursorInteractionComponent
	: public UWidgetInteractionComponent
{
	GENERATED_BODY()

public:
	/* Points the interaction at the mouse cursor */
	UStudyCursorInteractionComponent();

	/* Presses the left mouse button on the widget under the
	   cursor */
	void PressCursorButton();

	/* Releases the left mouse button */
	void ReleaseCursorButton();

	/* Scrolls the widget under the cursor */
	void ScrollCursorWheel(float WheelDelta);

	/* Types a keyboard key into the widget under the cursor */
	void TypeKey(const FKey& Key);

private:
	/* Returns whether the key is a printable character */
	static bool IsPrintableCharacter(uint32 CharacterCode);

	/* Returns the character the key types with the current Shift
	   and Caps Lock state, such as ^ for Shift and 6 */
	static TCHAR ApplyModifierKeys(TCHAR BaseCharacter);
};
