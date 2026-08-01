#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CaptiveSkyConversationWidget.generated.h"

class SEditableTextBox;
class STextBlock;

UCLASS()
class CAPTIVESKY_2_API UCaptiveSkyConversationWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void OpenFor(const FString& AgentName);
	void ShowMessage(const FString& Message, bool bEnableInput);
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
private:
	TSharedPtr<STextBlock> StatusText;
	TSharedPtr<SEditableTextBox> InputBox;
	void HandleTextCommitted(const FText& Text, ETextCommit::Type CommitType);
};
