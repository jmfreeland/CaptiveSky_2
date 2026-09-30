#include "CaptiveSkyAmbientSpeechWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Styling/AppStyle.h"

TSharedRef<SWidget> UCaptiveSkyAmbientSpeechWidget::RebuildWidget()
{
	return SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(FMargin(24.f, 24.f, 24.f, 150.f))
	[
		SNew(SBorder).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder")).Padding(FMargin(16.f, 10.f))
		[
			SAssignNew(SpeechText, STextBlock).AutoWrapText(true).WrapTextAt(700.f)
		]
	];
}

void UCaptiveSkyAmbientSpeechWidget::ShowSpeech(const FString& SpeakerName, const FString& Speech)
{
	ShowCaption(FString::Printf(TEXT("%s: %s"), *SpeakerName, *Speech));
}

void UCaptiveSkyAmbientSpeechWidget::ShowCaption(const FString& Caption)
{
	DisplayedCaption = Caption;
	RefreshDisplayedText();
}

void UCaptiveSkyAmbientSpeechWidget::HideCaption()
{
	DisplayedCaption.Reset();
	RefreshDisplayedText();
}

void UCaptiveSkyAmbientSpeechWidget::ShowInteractionHint(const FString& Hint)
{
	if (DisplayedInteractionHint == Hint) return;
	DisplayedInteractionHint = Hint;
	RefreshDisplayedText();
}

void UCaptiveSkyAmbientSpeechWidget::HideInteractionHint()
{
	if (DisplayedInteractionHint.IsEmpty()) return;
	DisplayedInteractionHint.Reset();
	RefreshDisplayedText();
}

void UCaptiveSkyAmbientSpeechWidget::RefreshDisplayedText()
{
	FString DisplayText = DisplayedInteractionHint;
	if (!DisplayText.IsEmpty() && !DisplayedCaption.IsEmpty()) DisplayText += LINE_TERMINATOR;
	DisplayText += DisplayedCaption;
	if (SpeechText.IsValid()) SpeechText->SetText(FText::FromString(DisplayText));
	SetVisibility(DisplayText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}

FText UCaptiveSkyAmbientSpeechWidget::GetDisplayedCaption() const
{
	return FText::FromString(DisplayedCaption);
}

FText UCaptiveSkyAmbientSpeechWidget::GetDisplayedInteractionHint() const
{
	return FText::FromString(DisplayedInteractionHint);
}
