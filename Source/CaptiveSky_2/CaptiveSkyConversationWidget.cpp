#include "CaptiveSkyConversationWidget.h"
#include "CaptiveSky_2PlayerController.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Styling/AppStyle.h"

TSharedRef<SWidget> UCaptiveSkyConversationWidget::RebuildWidget()
{
	return SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(FMargin(24.f, 24.f, 24.f, 64.f))
	[
		SNew(SBorder).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder")).Padding(16.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 10.f)
			[
				SAssignNew(StatusText, STextBlock).AutoWrapText(true).WrapTextAt(620.f)
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SAssignNew(InputBox, SEditableTextBox).MinDesiredWidth(620.f)
				.HintText(FText::FromString(TEXT("Say something…")))
				.OnTextCommitted_UObject(this, &UCaptiveSkyConversationWidget::HandleTextCommitted)
			]
		]
	];
}

void UCaptiveSkyConversationWidget::OpenFor(const FString& AgentName)
{
	SetVisibility(ESlateVisibility::Visible);
	ShowMessage(FString::Printf(TEXT("Speak to %s — Enter sends, Escape leaves."), *AgentName), true);
	if (InputBox.IsValid())
	{
		InputBox->SetText(FText::GetEmpty());
		FSlateApplication::Get().SetKeyboardFocus(InputBox, EFocusCause::SetDirectly);
	}
}

void UCaptiveSkyConversationWidget::ShowMessage(const FString& Message, bool bEnableInput)
{
	if (StatusText.IsValid()) StatusText->SetText(FText::FromString(Message));
	if (InputBox.IsValid())
	{
		InputBox->SetEnabled(bEnableInput);
		if (bEnableInput) FSlateApplication::Get().SetKeyboardFocus(InputBox, EFocusCause::SetDirectly);
	}
}

void UCaptiveSkyConversationWidget::HandleTextCommitted(const FText& Text, ETextCommit::Type CommitType)
{
	if (CommitType != ETextCommit::OnEnter || Text.IsEmpty()) return;
	if (ACaptiveSky_2PlayerController* Controller = Cast<ACaptiveSky_2PlayerController>(GetOwningPlayer()))
	{
		InputBox->SetText(FText::GetEmpty());
		Controller->SubmitConversation(Text.ToString());
	}
}
