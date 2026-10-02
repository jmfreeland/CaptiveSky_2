#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CaptiveSkyGuestBookWidget.generated.h"

class SEditableTextBox;
class SButton;
class STextBlock;
class FReply;

/** Small public writing panel for the Island's shared, persistent inn guest book. */
UCLASS()
class CAPTIVESKY_2_API UCaptiveSkyGuestBookWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void OpenFor(const FString& LatestEntries, bool bCanWriteToday);
	void ShowWriteResult(const FString& Result, const FString& LatestEntries, bool bCanWriteToday, bool bClearInput);
	TSharedPtr<SWidget> GetPreferredInputWidget() const;
	FString GetDisplayedContent() const { return DisplayedContent; }
	bool CanWriteToday() const { return bMayWriteToday; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	friend class FIslandGuestBookTest;

	TSharedPtr<STextBlock> StatusText;
	TSharedPtr<SEditableTextBox> InputBox;
	TSharedPtr<SButton> CloseButton;
	FString DisplayedContent;
	bool bMayWriteToday = true;

	void SubmitCurrentText();
	void HandleTextCommitted(const FText& Text, ETextCommit::Type CommitType);
	FReply HandleSubmitClicked();
	FReply HandleCloseClicked();
};
