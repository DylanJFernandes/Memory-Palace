// Copyright Epic Games, Inc. All Rights Reserved.

#include "Component/StudyCursorInteractionComponent.h"
#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"

namespace
{
	constexpr float CursorInteractionDistance{1000.0f};
	constexpr uint32 FirstPrintableCharacterCode{32};
	constexpr uint32 DeleteCharacterCode{127};
	const FString UnshiftedSymbols{TEXT("1234567890-=[]\\;',./`")};
	const FString ShiftedSymbols{TEXT("!@#$%^&*()_+{}|:\"<>?~")};
}

UStudyCursorInteractionComponent::UStudyCursorInteractionComponent()
{
	InteractionSource = EWidgetInteractionSource::Mouse;
	InteractionDistance = CursorInteractionDistance;
}

void UStudyCursorInteractionComponent::PressCursorButton()
{
	PressPointerKey(EKeys::LeftMouseButton);
}

void UStudyCursorInteractionComponent::ReleaseCursorButton()
{
	ReleasePointerKey(EKeys::LeftMouseButton);
}

void UStudyCursorInteractionComponent::ScrollCursorWheel(
	float WheelDelta)
{
	ScrollWheel(WheelDelta);
}

void UStudyCursorInteractionComponent::TypeKey(const FKey& Key)
{
	const uint32* KeyCode{nullptr};
	const uint32* CharacterCode{nullptr};
	FInputKeyManager::Get().GetCodesFromKey(
		Key, KeyCode, CharacterCode);
	if (!CharacterCode || !IsPrintableCharacter(*CharacterCode))
	{
		PressAndReleaseKey(Key);
		return;
	}
	TCHAR TypedCharacter{
		ApplyModifierKeys(static_cast<TCHAR>(*CharacterCode))};
	SendKeyChar(FString{1, &TypedCharacter});
}

bool UStudyCursorInteractionComponent::IsPrintableCharacter(
	uint32 CharacterCode)
{
	return CharacterCode >= FirstPrintableCharacterCode
		&& CharacterCode != DeleteCharacterCode;
}

TCHAR UStudyCursorInteractionComponent::ApplyModifierKeys(
	TCHAR BaseCharacter)
{
	const FModifierKeysState ModifierKeys{
		FSlateApplication::Get().GetModifierKeys()};
	const bool bIsShiftHeld{ModifierKeys.IsShiftDown()};
	if (FChar::IsAlpha(BaseCharacter))
	{
		const bool bIsUppercase{
			bIsShiftHeld != ModifierKeys.AreCapsLocked()};
		return bIsUppercase ? FChar::ToUpper(BaseCharacter)
							: FChar::ToLower(BaseCharacter);
	}
	if (!bIsShiftHeld)
	{
		return BaseCharacter;
	}
	int32 SymbolIndex{INDEX_NONE};
	if (!UnshiftedSymbols.FindChar(BaseCharacter, SymbolIndex))
	{
		return BaseCharacter;
	}
	return ShiftedSymbols[SymbolIndex];
}
