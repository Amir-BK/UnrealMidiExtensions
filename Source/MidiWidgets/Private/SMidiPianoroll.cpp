// Fill out your copyright notice in the Description page of Project Settings.


#include "SMidiPianoroll.h"
#include "SlateOptMacros.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

SLATE_IMPLEMENT_WIDGET(SMidiPianoroll)
void SMidiPianoroll::PrivateRegisterAttributes(FSlateAttributeInitializer& AttributeInitializer)
{
	SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION_WITH_NAME(AttributeInitializer, "Offset", Offset, EInvalidateWidgetReason::Paint);
	SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION_WITH_NAME(AttributeInitializer, "Zoom", Zoom, EInvalidateWidgetReason::Paint);
}

SMidiPianoroll::SMidiPianoroll()
	: Offset(*this, FVector2D::ZeroVector)
	, Zoom(*this, FVector2D(1.0f, 1.0f))
{
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SMidiPianoroll::Construct(const FArguments& InArgs)
{
	/*
	ChildSlot
	[
		// Populate the widget
	];
	*/

	LinkedMidiData = InArgs._LinkedMidiData;
	LinkedSongsMap = InArgs._LinkedSongsMap;
	VisualizationData = InArgs._VisualizationData;
	PianorollStyle = InArgs._PianorollStyle ? InArgs._PianorollStyle : &FMidiPianorollStyle::GetDefault();

	Offset.Assign(*this, InArgs._Offset);
	Zoom.Assign(*this, InArgs._Zoom);

	ChildSlot
	[
		SNullWidget::NullWidget
	];
}
int32 SMidiPianoroll::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    const FSlateBrush* Brush = FCoreStyle::Get().GetBrush("WhiteBrush");

    const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
    const FVector2D LocalOffset = Offset.Get();
    const FVector2D LocalZoom = Zoom.Get();

    // Background
    FSlateDrawElement::MakeBox(
        OutDrawElements,
        LayerId++,
        AllottedGeometry.ToPaintGeometry(LocalSize, FSlateLayoutTransform()),
        Brush,
        ESlateDrawEffect::None,
        FLinearColor(0.05f, 0.05f, 0.05f, 1.0f));

    if(LinkedMidiData.IsValid())
    {
        for (int32 TrackIdx = 0; TrackIdx < LinkedMidiData->Tracks.Num(); ++TrackIdx)
        {
            const FMidiNotesTrack& Track = LinkedMidiData->Tracks[TrackIdx];
            // Track color from visualization data if available
            FLinearColor TrackColor = FLinearColor::White;
            if (VisualizationData.IsValid() && VisualizationData->TrackVisualizations.IsValidIndex(TrackIdx))
            {
                const FMidiTrackVisualizationData& Vis = VisualizationData->TrackVisualizations[TrackIdx];
                if (!Vis.bIsVisible) { continue; }
                TrackColor = Vis.TrackColor;
            }

            for (const FLinkedMidiNote& Note : Track.Notes)
            {
                const float X = (Note.NoteOnTick * 0.5f) * LocalZoom.X - LocalOffset.X;
                const float W = FMath::Max((Note.NoteOffTick - Note.NoteOnTick) * 0.5f, 1.0f) * LocalZoom.X;
                const float RowH = 10.0f * LocalZoom.Y;
                const float Y = (127 - Note.NoteNumber) * (RowH + 2.0f) - LocalOffset.Y;

                if (X > LocalSize.X || X + W < 0.0f || Y > LocalSize.Y || Y + RowH < 0.0f)
                {
                    continue;
                }

                FSlateDrawElement::MakeBox(
                    OutDrawElements,
                    LayerId,
                    AllottedGeometry.ToPaintGeometry(FVector2D(W, RowH), FSlateLayoutTransform(FVector2D(X, Y))),
                    Brush,
                    ESlateDrawEffect::None,
                    TrackColor.CopyWithNewOpacity(0.9f));
            }
        }
    }

    return LayerId;
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION
