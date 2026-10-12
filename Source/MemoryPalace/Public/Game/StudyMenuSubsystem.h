// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "StudyMenuSubsystem.generated.h"

class UStudyMenuSaveGame;
class UTexture2D;

/* Fires with the QR code that opens the upload page */
DECLARE_DELEGATE_OneParam(FStudyQrCodeReceived, UTexture2D*);

/* Fires with the world-building progress (0 to 1), its status
   word and a message for the player */
DECLARE_DELEGATE_ThreeParams(FStudyMaterialsStatusReceived, float,
							 const FString&, const FString&);

/* Keeps the menu's data across levels: the saved worlds and the
   API key, and talks to the server while a world is created */
UCLASS()
class MEMORYPALACE_API UStudyMenuSubsystem
	: public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/* Loads the saved worlds and API key */
	virtual void Initialize(
		FSubsystemCollectionBase& Collection) override;

	/* Returns the names of the saved worlds */
	const TArray<FString>& GetWorldNames() const;

	/* Adds a world to the list and saves it */
	void AddWorld(const FString& WorldName);

	/* Removes a world from the list and saves it */
	void RemoveWorld(const FString& WorldName);

	/* Returns the saved API key */
	const FString& GetApiKey() const;

	/* Saves the API key and sends it to the server */
	void SaveApiKey(const FString& InApiKey);

	/* Downloads a QR code of the server's upload page */
	void RequestQrCode();

	/* Asks the server to build the problems from the upload */
	void RequestGeneration();

	/* Asks the server how far world building has come */
	void RequestMaterialsStatus();

	/* Fires when the QR code has been downloaded */
	FStudyQrCodeReceived OnQrCodeReceived;

	/* Fires when the building status arrives */
	FStudyMaterialsStatusReceived OnMaterialsStatusReceived;

private:
	/* Writes the menu data to disk */
	void SaveToDisk() const;

	/* Sends the API key to the server */
	void SendApiKeyToServer() const;

	/* Reads the upload page's address and downloads its QR code */
	void HandleUploadUrlResponse(FHttpRequestPtr Request,
								 FHttpResponsePtr Response,
								 bool bConnectedSuccessfully);

	/* Turns the QR code image into a texture and announces it */
	void HandleQrCodeResponse(FHttpRequestPtr Request,
							  FHttpResponsePtr Response,
							  bool bConnectedSuccessfully);

	/* Reads the building status and announces it */
	void HandleMaterialsStatusResponse(
		FHttpRequestPtr Request, FHttpResponsePtr Response,
		bool bConnectedSuccessfully);

	/* Builds a request to the study pad server */
	static TSharedRef<IHttpRequest, ESPMode::ThreadSafe>
		CreateServerRequest(const TCHAR* Verb, const FString& Path);

	/* Menu data loaded from and saved to disk */
	UPROPERTY()
	TObjectPtr<UStudyMenuSaveGame> MenuSaveGame;

	/* QR code texture, kept so it is not garbage collected */
	UPROPERTY()
	TObjectPtr<UTexture2D> QrCodeTexture;
};
