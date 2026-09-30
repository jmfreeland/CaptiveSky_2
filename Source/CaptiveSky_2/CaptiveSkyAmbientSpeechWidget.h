#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CaptiveSkyAmbientSpeechWidget.generated.h"

class STextBlock;

UCLASS()
class CAPTIVESKY_2_API UCaptiveSkyAmbientSpeechWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ShowSpeech(const FString& SpeakerName, const FString& Speech);
	void ShowCaption(const FString& Caption);
	void HideCaption();
	void ShowInteractionHint(const FString& Hint);
	void HideInteractionHint();
	FText GetDisplayedCaption() const;
	FText GetDisplayedInteractionHint() const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	TSharedPtr<STextBlock> SpeechText;
	FString DisplayedCaption;
	FString DisplayedInteractionHint;
	void RefreshDisplayedText();
};
