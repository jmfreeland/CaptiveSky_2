#include "CaptiveSkyGuestBookWidget.h"

#include "CaptiveSky_2PlayerController.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Styling/AppStyle.h"

TSharedRef<SWidget> UCaptiveSkyGuestBookWidget::RebuildWidget()
{
	return SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(FMargin(24.f, 24.f, 24.f, 64.f))
	[
		SNew(SBorder).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder")).Padding(16.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 10.f)
			[
				SAssignNew(StatusText, STextBlock).AutoWrapText(true).WrapTextAt(680.f)
				.Text(FText::FromString(DisplayedContent))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
			[
				SAssignNew(InputBox, SEditableTextBox).MinDesiredWidth(680.f)
				.HintText(FText::FromString(TEXT("Write one short line for the Island...")))
				.IsEnabled(bMayWriteToday)
				.OnTextCommitted_UObject(this, &UCaptiveSkyGuestBookWidget::HandleTextCommitted)
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 8.f, 0.f)
				[
					SNew(SButton).Text(FText::FromString(TEXT("Leave this line")))
					.OnClicked_UObject(this, &UCaptiveSkyGuestBookWidget::HandleSubmitClicked)
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton).Text(FText::FromString(TEXT("Close (Esc)")))
					.OnClicked_UObject(this, &UCaptiveSkyGuestBookWidget::HandleCloseClicked)
				]
			]
		]
	];
}

void UCaptiveSkyGuestBookWidget::OpenFor(const FString& LatestEntries, bool bCanWriteToday)
{
	bMayWriteToday = bCanWriteToday;
	SetVisibility(ESlateVisibility::Visible);
	const FString PrivacyNote = TEXT("Notes are public, signed as Visitor, and may be read by the Island's residents. One line per Island day; 180 characters maximum. Do not put private information here.");
	DisplayedContent = LatestEntries + TEXT("\n\n") + (bCanWriteToday
		? PrivacyNote + TEXT("\nEnter saves; Escape closes.")
		: TEXT("You have already left a line today. You may read the book again tomorrow.\nEscape closes."));
	if (StatusText.IsValid()) StatusText->SetText(FText::FromString(DisplayedContent));
	if (InputBox.IsValid())
	{
		InputBox->SetEnabled(bCanWriteToday);
		InputBox->SetText(FText::GetEmpty());
		if (bCanWriteToday && FSlateApplication::IsInitialized())
			FSlateApplication::Get().SetKeyboardFocus(InputBox, EFocusCause::SetDirectly);
	}
}

void UCaptiveSkyGuestBookWidget::ShowWriteResult(const FString& Result, const FString& LatestEntries, bool bCanWriteToday, bool bClearInput)
{
	bMayWriteToday = bCanWriteToday;
	const FString NextStep = bCanWriteToday
		? TEXT("One public line per Island day, up to 180 characters. Enter saves; Escape closes.")
		: TEXT("You have already left today's line. Anyone can read it; come back another Island day.");
	DisplayedContent = Result + TEXT("\n\n") + LatestEntries + TEXT("\n\n") + NextStep;
	if (StatusText.IsValid()) StatusText->SetText(FText::FromString(DisplayedContent));
	if (InputBox.IsValid())
	{
		InputBox->SetEnabled(bCanWriteToday);
		if (bClearInput) InputBox->SetText(FText::GetEmpty());
		if (bCanWriteToday && FSlateApplication::IsInitialized())
			FSlateApplication::Get().SetKeyboardFocus(InputBox, EFocusCause::SetDirectly);
	}
}

void UCaptiveSkyGuestBookWidget::SubmitCurrentText()
{
	if (ACaptiveSky_2PlayerController* Controller = Cast<ACaptiveSky_2PlayerController>(GetOwningPlayer()))
		Controller->SubmitGuestBookEntry(InputBox.IsValid() ? InputBox->GetText().ToString() : FString());
}

void UCaptiveSkyGuestBookWidget::HandleTextCommitted(const FText& Text, ETextCommit::Type CommitType)
{
	if (CommitType == ETextCommit::OnEnter) SubmitCurrentText();
}

FReply UCaptiveSkyGuestBookWidget::HandleSubmitClicked()
{
	SubmitCurrentText();
	return FReply::Handled();
}

FReply UCaptiveSkyGuestBookWidget::HandleCloseClicked()
{
	if (ACaptiveSky_2PlayerController* Controller = Cast<ACaptiveSky_2PlayerController>(GetOwningPlayer()))
		Controller->CloseGuestBook();
	return FReply::Handled();
}
