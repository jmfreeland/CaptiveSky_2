#include "CaptiveSkyArrangementWidget.h"

#include "CaptiveSky_2PlayerController.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Styling/AppStyle.h"

TSharedRef<SWidget> UCaptiveSkyArrangementWidget::RebuildWidget()
{
	return SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(FMargin(24.f, 24.f, 0.f, 64.f))
	[
		SNew(SBorder).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder")).Padding(16.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
			[
				SAssignNew(StatusText, STextBlock).AutoWrapText(true).WrapTextAt(680.f)
				.Text(FText::FromString(DisplayedContent))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
			[
				SAssignNew(TitleBox, SEditableTextBox).MinDesiredWidth(640.f)
				.HintText(FText::FromString(TEXT("A private title (optional, 60 characters)...")))
				.Visibility_Lambda([this]() { return bSiteHasWork ? EVisibility::Collapsed : EVisibility::Visible; })
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
			[
				SAssignNew(IntentBox, SEditableTextBox).MinDesiredWidth(640.f)
				.HintText(FText::FromString(TEXT("What it means to you (optional, 200 characters)...")))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 6.f, 0.f)
				[
					SNew(SButton).Text(FText::FromString(TEXT("Ring"))).IsEnabled_Lambda([this]() { return bMayContribute && !bSiteHasWork; })
					.OnClicked_Lambda([this]() { SubmitArrangement(TEXT("ring")); return FReply::Handled(); })
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 6.f, 0.f)
				[
					SNew(SButton).Text(FText::FromString(TEXT("Line"))).IsEnabled_Lambda([this]() { return bMayContribute && !bSiteHasWork; })
					.OnClicked_Lambda([this]() { SubmitArrangement(TEXT("line")); return FReply::Handled(); })
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 6.f, 0.f)
				[
					SNew(SButton).Text(FText::FromString(TEXT("Spiral"))).IsEnabled_Lambda([this]() { return bMayContribute && !bSiteHasWork; })
					.OnClicked_Lambda([this]() { SubmitArrangement(TEXT("spiral")); return FReply::Handled(); })
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 6.f, 0.f)
				[
					SNew(SButton).Text(FText::FromString(TEXT("Pair"))).IsEnabled_Lambda([this]() { return bMayContribute && !bSiteHasWork; })
					.OnClicked_Lambda([this]() { SubmitArrangement(TEXT("pair")); return FReply::Handled(); })
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(12.f, 0.f, 6.f, 0.f)
				[
					SNew(SButton).Text(FText::FromString(TEXT("Add a response")))
					.Visibility_Lambda([this]() { return bSiteHasWork ? EVisibility::Visible : EVisibility::Collapsed; })
					.IsEnabled_Lambda([this]() { return bMayRespond; })
					.OnClicked_Lambda([this]() { SubmitResponse(); return FReply::Handled(); })
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton).Text(FText::FromString(TEXT("Close (Esc)")))
					.OnClicked_UObject(this, &UCaptiveSkyArrangementWidget::HandleCloseClicked)
				]
			]
		]
	];
}

void UCaptiveSkyArrangementWidget::OpenFor(FName InSiteId, const FString& Description, bool bHasWork,
	bool bCanCreate, bool bCanRespond, bool bOwnWork)
{
	SiteId = InSiteId;
	bSiteHasWork = bHasWork;
	bMayContribute = bCanCreate;
	bMayRespond = bCanRespond;
	const FString Availability = bOwnWork
		? TEXT("This is your own work; it will remain as you left it.")
		: bCanCreate || bCanRespond
			? TEXT("One creation or response per Island day. Your title and meaning are private unless you share them.")
			: TEXT("You have already contributed today, or this work has no room for another response. You can return on another Island day.");
	DisplayedContent = Description + TEXT("\n\n") + Availability + TEXT("\nThe stone shape is public and persists with the Island; only the maker sees its title and meaning. The developer reset can clear prototype arrangements.");
	SetVisibility(ESlateVisibility::Visible);
	if (StatusText.IsValid()) StatusText->SetText(FText::FromString(DisplayedContent));
	if (TitleBox.IsValid())
	{
		TitleBox->SetText(FText::GetEmpty());
		if (!bHasWork && bCanCreate && FSlateApplication::IsInitialized())
			FSlateApplication::Get().SetKeyboardFocus(TitleBox, EFocusCause::SetDirectly);
	}
	if (IntentBox.IsValid()) IntentBox->SetText(FText::GetEmpty());
}

void UCaptiveSkyArrangementWidget::ShowResult(const FString& Result, bool bHasWork, bool bCanCreate, bool bCanRespond, bool bClearInput)
{
	bSiteHasWork = bHasWork;
	bMayContribute = bCanCreate;
	bMayRespond = bCanRespond;
	DisplayedContent = Result + TEXT("\n\n") + (CanContribute()
		? TEXT("You may make one creation or response per Island day. Escape closes.")
		: TEXT("You have used today's contribution, or this work is complete. Escape closes."));
	if (StatusText.IsValid()) StatusText->SetText(FText::FromString(DisplayedContent));
	if (bClearInput && TitleBox.IsValid()) TitleBox->SetText(FText::GetEmpty());
	if (bClearInput && IntentBox.IsValid()) IntentBox->SetText(FText::GetEmpty());
}

void UCaptiveSkyArrangementWidget::SubmitArrangement(const FString& Form)
{
	if (ACaptiveSky_2PlayerController* Controller = Cast<ACaptiveSky_2PlayerController>(GetOwningPlayer()))
		Controller->SubmitVisitorArrangement(Form, TitleBox.IsValid() ? TitleBox->GetText().ToString() : FString(),
			IntentBox.IsValid() ? IntentBox->GetText().ToString() : FString());
}

void UCaptiveSkyArrangementWidget::SubmitResponse()
{
	if (ACaptiveSky_2PlayerController* Controller = Cast<ACaptiveSky_2PlayerController>(GetOwningPlayer()))
		Controller->SubmitVisitorArrangementResponse(IntentBox.IsValid() ? IntentBox->GetText().ToString() : FString());
}

FReply UCaptiveSkyArrangementWidget::HandleCloseClicked()
{
	if (ACaptiveSky_2PlayerController* Controller = Cast<ACaptiveSky_2PlayerController>(GetOwningPlayer()))
		Controller->CloseArrangement();
	return FReply::Handled();
}
