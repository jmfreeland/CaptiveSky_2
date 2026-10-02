#pragma once

#include "CoreMinimal.h"
#include "CaptiveSky_2PlayerController.h"
#include "Components/InputComponent.h"
#include "InputCoreTypes.h"
#include "IslandInteractionTestPlayerController.generated.h"

/** Minimal local-controller shim for exercising the bound E action in automation fixtures. */
UCLASS(Transient, NotBlueprintable)
class AIslandInteractionTestPlayerController final : public ACaptiveSky_2PlayerController
{
	GENERATED_BODY()

public:
	virtual bool IsLocalController() const override { return true; }
	virtual void SetGuestBookInputMode(bool bOpen) override { ACaptiveSky_2PlayerController::SetGuestBookInputMode(bOpen); }
	virtual void SetArrangementInputMode(bool bOpen) override { ACaptiveSky_2PlayerController::SetArrangementInputMode(bOpen); }

	void SetFixturePawn(APawn* InPawn) { SetPawn(InPawn); }
	void SetCaptionWidget(UCaptiveSkyAmbientSpeechWidget* InWidget) { AmbientSpeechWidget = InWidget; }
	void SetGuestBookWidget(UCaptiveSkyGuestBookWidget* InWidget) { GuestBookWidget = InWidget; }
	void SetArrangementWidget(UCaptiveSkyArrangementWidget* InWidget) { ArrangementWidget = InWidget; }
	bool IsGuestBookPanelOpenForTest() const { return bGuestBookPanelOpen; }
	bool IsArrangementPanelOpenForTest() const { return bArrangementPanelOpen; }
	void BindFixtureInput() { SetupInputComponent(); }
	void RefreshInteractionHintForTest() { UpdateWorldInteractionHint(); }

	bool PressBoundE()
	{
		if (!InputComponent) return false;
		for (const FInputKeyBinding& Binding : InputComponent->KeyBindings)
		{
			if (Binding.KeyEvent == IE_Pressed && Binding.Chord.Key == EKeys::E && Binding.KeyDelegate.IsBoundToObject(this))
			{
				Binding.KeyDelegate.Execute(EKeys::E);
				return true;
			}
		}
		return false;
	}

	bool PressBoundEscape()
	{
		if (!InputComponent) return false;
		for (const FInputKeyBinding& Binding : InputComponent->KeyBindings)
		{
			if (Binding.KeyEvent == IE_Pressed && Binding.Chord.Key == EKeys::Escape && Binding.KeyDelegate.IsBoundToObject(this))
			{
				Binding.KeyDelegate.Execute(EKeys::Escape);
				return true;
			}
		}
		return false;
	}
};
