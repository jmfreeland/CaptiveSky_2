#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CaptiveSkyArrangementWidget.generated.h"

class SEditableTextBox;
class STextBlock;
class FReply;

/** Small public panel for leaving a persistent stone work or a response beside one. */
UCLASS()
class CAPTIVESKY_2_API UCaptiveSkyArrangementWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void OpenFor(FName InSiteId, const FString& Description, bool bHasWork, bool bMayContribute,
		bool bMayRespond, bool bOwnWork);
	void ShowResult(const FString& Result, bool bHasWork, bool bMayContribute, bool bMayRespond, bool bClearInput);
	TSharedPtr<SWidget> GetPreferredInputWidget() const;
	FString GetDisplayedContent() const { return DisplayedContent; }
	bool CanContribute() const { return bMayContribute || bMayRespond; }
	FName GetSiteId() const { return SiteId; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	friend class FIslandArrangementTest;

	TSharedPtr<STextBlock> StatusText;
	TSharedPtr<SEditableTextBox> TitleBox;
	TSharedPtr<SEditableTextBox> IntentBox;
	FString DisplayedContent;
	FName SiteId;
	bool bSiteHasWork = false;
	bool bMayContribute = false;
	bool bMayRespond = false;

	void SubmitArrangement(const FString& Form);
	void SubmitResponse();
	FReply HandleCloseClicked();
};
