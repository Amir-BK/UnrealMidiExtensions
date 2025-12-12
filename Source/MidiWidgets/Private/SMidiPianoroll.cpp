// Fill out your copyright notice in the Description page of Project Settings.


#include "SMidiPianoroll.h"
#include "SlateOptMacros.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"


namespace
{
    constexpr bool IsAccidentalNote(int32 NoteNumber)
    {
        return NoteNumber % 12 == 1 || NoteNumber % 12 == 3 || NoteNumber % 12 == 6 || NoteNumber % 12 == 8 || NoteNumber % 12 == 10;
    }
};

SLATE_IMPLEMENT_WIDGET(SMidiPianoroll)
void SMidiPianoroll::PrivateRegisterAttributes(FSlateAttributeInitializer& AttributeInitializer)
{
	SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION_WITH_NAME(AttributeInitializer, "Offset", Offset, EInvalidateWidgetReason::Paint);
	SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION_WITH_NAME(AttributeInitializer, "Zoom", Zoom, EInvalidateWidgetReason::Paint);
	SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION_WITH_NAME(AttributeInitializer, "VisualizationData", VisualizationData, EInvalidateWidgetReason::Paint);
}

SMidiPianoroll::SMidiPianoroll()
	: Offset(*this, FVector2D::ZeroVector)
	, Zoom(*this, FVector2D(1.0f, 1.0f))
	, VisualizationData(*this, FMidiFileVisualizationData())
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
	PianorollStyle = InArgs._PianorollStyle ? InArgs._PianorollStyle : &FMidiPianorollStyle::GetDefault();

	Offset.Assign(*this, InArgs._Offset);
	Zoom.Assign(*this, InArgs._Zoom);
	VisualizationData.Assign(*this, InArgs._VisualizationData);

	ChildSlot
	[
		SNullWidget::NullWidget
	];
}
int32 SMidiPianoroll::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    const FSlateBrush* Brush = &PianorollStyle->GridBrush;
	const FSlateBrush* NoteBrush = &PianorollStyle->NoteBrush;

    const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
    const FVector2D LocalOffset = Offset.Get();
    const FVector2D LocalZoom = Zoom.Get();

    // Background

	const FLinearColor& GridColor = PianorollStyle->GridColor;
	const FLinearColor& AccidentalGridColor = PianorollStyle->AccidentalGridColor;

    // Draw piano grid
    LayerId++;
    {
        const float RowH = 10.0f * LocalZoom.Y;
        
        for (int i = 0; i < 128; i++)
        {
            const float Y = (127 - i) * (RowH + 2.0f) - LocalOffset.Y;
            if (IsAccidentalNote(i))
            {
                FSlateDrawElement::MakeBox(
                    OutDrawElements,
                    LayerId,
                    AllottedGeometry.ToPaintGeometry(
                        FVector2D(LocalSize.X, RowH),
                        FSlateLayoutTransform(FVector2D(0.0f, Y))
                    ),
                    Brush,
                    ESlateDrawEffect::None,
					AccidentalGridColor);

            }
            else {
                FSlateDrawElement::MakeBox(
                    OutDrawElements,
                    LayerId,
                    AllottedGeometry.ToPaintGeometry(
                        FVector2D(LocalSize.X, RowH),
                        FSlateLayoutTransform(FVector2D(0.0f, Y))
                    ),
                    Brush,
					ESlateDrawEffect::None,
                    GridColor);

            }
        }

    }
   


    if(LinkedMidiData.IsValid())
    {
        const FMidiFileVisualizationData& VisData = VisualizationData.Get();
        
        for (int32 TrackIdx = 0; TrackIdx < LinkedMidiData->Tracks.Num(); ++TrackIdx)
        {
            const FMidiNotesTrack& Track = LinkedMidiData->Tracks[TrackIdx];
            // Track color from visualization data if available
            FLinearColor TrackColor = FLinearColor::White;
            if (VisData.TrackVisualizations.IsValidIndex(TrackIdx))
            {
                const FMidiTrackVisualizationData& Vis = VisData.TrackVisualizations[TrackIdx];
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
                    NoteBrush,
                    ESlateDrawEffect::None,
                    TrackColor);
            }
        }
    }

    return LayerId;
}
FReply SMidiPianoroll::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{

    if (bIsRightMouseButtonDown)
    {
        const FVector2D CurrentPos = Offset.Get();
        const FVector2D Delta = MouseEvent.GetCursorDelta();
        if (!Delta.IsNearlyZero())
        {
            Offset.Set(*this, CurrentPos - Delta / Zoom.Get());
            bIsPanning = true;
            return FReply::Handled();
        }
        else {
            bIsPanning = false;
        }
    }
 


    return FReply::Unhandled();
}
FReply SMidiPianoroll::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        // Handle left mouse button down
    }

    if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
    {
        bIsRightMouseButtonDown = true;
        return FReply::Handled().CaptureMouse(SharedThis(this));
	}

	return FReply::Unhandled();
}
FReply SMidiPianoroll::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	//if right mouse button up
    if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
    {
        bIsRightMouseButtonDown = false;
        bIsPanning = false;
        return FReply::Handled().ReleaseMouseCapture();
	}
    
	return FReply::Unhandled();
}
FReply SMidiPianoroll::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const bool bIsCtrlDown = MouseEvent.IsControlDown();
    if (bIsCtrlDown)
    {
        const float WheelDelta = MouseEvent.GetWheelDelta();
        FVector2D CurrentZoom = Zoom.Get();
        const float ZoomFactor = 0.1f;
        if (WheelDelta > 0)
        {
            CurrentZoom *= (1.0f + ZoomFactor);
        }
        else if (WheelDelta < 0)
        {
            CurrentZoom *= (1.0f - ZoomFactor);
        }
        // Clamp zoom levels
        CurrentZoom.X = FMath::Clamp(CurrentZoom.X, 0.1f, 10.0f);
        CurrentZoom.Y = FMath::Clamp(CurrentZoom.Y, 0.1f, 10.0f);
        Zoom.Set(*this, CurrentZoom);
		return FReply::Handled();
    }
    
    return FReply::Unhandled();
}
TOptional<EMouseCursor::Type> SMidiPianoroll::GetCursor() const
{
    if (bIsRightMouseButtonDown)
    {
        if (bIsPanning)
        {
            return EMouseCursor::GrabHandClosed;
        }
		return EMouseCursor::GrabHand;
    }

	return EMouseCursor::Default;
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION
