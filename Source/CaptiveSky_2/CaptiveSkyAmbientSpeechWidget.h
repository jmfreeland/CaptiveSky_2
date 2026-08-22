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

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	TSharedPtr<STextBlock> SpeechText;
};
