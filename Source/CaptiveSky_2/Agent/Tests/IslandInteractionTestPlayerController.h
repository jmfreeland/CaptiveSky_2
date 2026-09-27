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

	void SetFixturePawn(APawn* InPawn) { SetPawn(InPawn); }
	void SetCaptionWidget(UCaptiveSkyAmbientSpeechWidget* InWidget) { AmbientSpeechWidget = InWidget; }
	void BindFixtureInput() { SetupInputComponent(); }

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
};
