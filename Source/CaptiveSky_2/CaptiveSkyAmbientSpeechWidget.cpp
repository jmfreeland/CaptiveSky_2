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
	if (SpeechText.IsValid())
	{
		SpeechText->SetText(FText::FromString(FString::Printf(TEXT("%s: %s"), *SpeakerName, *Speech)));
	}
	SetVisibility(ESlateVisibility::HitTestInvisible);
}
