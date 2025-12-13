// Fill out your copyright notice in the Description page of Project Settings.


#include "SMidiPianoroll.h"
#include "SlateOptMacros.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Styling/AppStyle.h"


namespace
{
    constexpr bool IsAccidentalNote(int32 NoteNumber)
    {
        return NoteNumber % 12 == 1 || NoteNumber % 12 == 3 || NoteNumber % 12 == 6 || NoteNumber % 12 == 8 || NoteNumber % 12 == 10;
    }

    // Constants for timeline marking density
    constexpr int32 MinPixelsPerBarText = 60;
};

SLATE_IMPLEMENT_WIDGET(SMidiPianoroll)
void SMidiPianoroll::PrivateRegisterAttributes(FSlateAttributeInitializer& AttributeInitializer)
{
	SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION_WITH_NAME(AttributeInitializer, "Offset", Offset, EInvalidateWidgetReason::Paint);
	SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION_WITH_NAME(AttributeInitializer, "Zoom", Zoom, EInvalidateWidgetReason::Paint);
	SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION_WITH_NAME(AttributeInitializer, "VisualizationData", VisualizationData, EInvalidateWidgetReason::Paint);
	SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION_WITH_NAME(AttributeInitializer, "TimeMode", TimeMode, EInvalidateWidgetReason::Paint);
}

SMidiPianoroll::SMidiPianoroll()
	: Offset(*this, FVector2D::ZeroVector)
	, Zoom(*this, FVector2D(1.0f, 1.0f))
	, VisualizationData(*this, FMidiFileVisualizationData())
	, TimeMode(*this, EMidiTrackTimeMode::TimeLinear)
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
	TimelineHeight = InArgs._TimelineHeight;

	Offset.Assign(*this, InArgs._Offset);
	Zoom.Assign(*this, InArgs._Zoom);
	VisualizationData.Assign(*this, InArgs._VisualizationData);
	TimeMode.Assign(*this, InArgs._TimeMode);

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

    // Recalculate grid points for current view
    RecalculateGrid(AllottedGeometry);

    // Layer 1: Draw piano grid rows (background)
    {
        const float RowH = 10.0f * LocalZoom.Y;
        const float ContentStartY = TimelineHeight; // Start below timeline
        
        for (int i = 0; i < 128; i++)
        {
            const float Y = ContentStartY + (127 - i) * (RowH + 2.0f) - LocalOffset.Y;
            
            // Skip rows that are above the timeline or below the visible area
            if (Y + RowH < TimelineHeight || Y > LocalSize.Y)
            {
                continue;
            }
            
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
            else 
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
                    GridColor);
            }
        }
    }
    LayerId++;

    // Layer 2: Paint vertical grid lines (on top of piano rows, behind notes)
    LayerId = PaintGridLines(AllottedGeometry, OutDrawElements, LayerId);

    // Layer 3: Draw MIDI notes
    if(LinkedMidiData.IsValid())
    {
        const FMidiFileVisualizationData& VisData = VisualizationData.Get();
        const float ContentStartY = TimelineHeight;
        
        for (int32 TrackIdx = 0; TrackIdx < LinkedMidiData->Tracks.Num(); ++TrackIdx)
        {
            const FMidiNotesTrack& Track = LinkedMidiData->Tracks[TrackIdx];
            // Track color from visualization data if available
            // Match by both TrackIndex AND ChannelIndex since one MIDI track can have multiple channels
            FLinearColor TrackColor = FLinearColor::White;
            int32 IndexOfTrackVis = VisData.TrackVisualizations.IndexOfByPredicate([&Track](const FMidiTrackVisualizationData& Data)
            {
                return Data.TrackIndex == Track.TrackIndex && Data.ChannelIndex == Track.ChannelIndex;
            });

            // If not found by exact match, fall back to just TrackIndex match (legacy/simple case)
            if (IndexOfTrackVis == INDEX_NONE)
            {
                IndexOfTrackVis = VisData.TrackVisualizations.IndexOfByPredicate([&Track](const FMidiTrackVisualizationData& Data)
                {
                    return Data.TrackIndex == Track.TrackIndex;
                });
            }

            const FMidiTrackVisualizationData* Vis = (IndexOfTrackVis != INDEX_NONE) 
                ? &VisData.TrackVisualizations[IndexOfTrackVis] 
                : nullptr;

            if (Vis)
            {
                if (!Vis->bIsVisible) { continue; }
                TrackColor = Vis->TrackColor;
            }

            for (const FLinkedMidiNote& Note : Track.Notes)
            {
                const float X = TickToPixel(Note.NoteOnTick) - LocalOffset.X;
                const float EndX = TickToPixel(Note.NoteOffTick) - LocalOffset.X;
                const float W = FMath::Max(EndX - X, 1.0f);
                const float RowH = 10.0f * LocalZoom.Y;
                const float Y = ContentStartY + (127 - Note.NoteNumber) * (RowH + 2.0f) - LocalOffset.Y;

                if (X > LocalSize.X || X + W < 0.0f || Y > LocalSize.Y || Y + RowH < TimelineHeight)
                {
                    continue;
                }

                // Use TrackIdx for layer ordering to ensure consistent z-order
                FSlateDrawElement::MakeBox(
                    OutDrawElements,
                    LayerId + TrackIdx,
                    AllottedGeometry.ToPaintGeometry(FVector2D(W, RowH), FSlateLayoutTransform(FVector2D(X, Y))),
                    NoteBrush,
                    ESlateDrawEffect::None,
                    TrackColor);
            }
        }
        LayerId += LinkedMidiData->Tracks.Num();
    }
    LayerId++;

    // Layer 4: Paint timeline header (on top of everything)
    LayerId = PaintTimeline(AllottedGeometry, OutDrawElements, LayerId);
 
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
            // Calculate new offset and clamp it
            const FVector2D NewOffset = CurrentPos - Delta;
            const FVector2D ClampedOffset = ClampOffset(NewOffset, MyGeometry.GetLocalSize());
            
            Offset.Set(*this, ClampedOffset);
            bIsPanning = true;
            return FReply::Handled();
        }
        else 
        {
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
    const bool bIsShiftDown = MouseEvent.IsShiftDown();
    const float WheelDelta = MouseEvent.GetWheelDelta();
    
    // Get current values
    FVector2D CurrentZoom = Zoom.Get();
    FVector2D CurrentOffset = Offset.Get();
    
    // Calculate mouse position in local widget space
    const FVector2D LocalMousePos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
    
    // Calculate the content position under the mouse cursor before zoom
    // Screen position = ContentPos * Zoom - Offset
    // ContentPos = (ScreenPos + Offset) / Zoom
    const FVector2D ContentPosUnderMouse = (LocalMousePos + CurrentOffset) / CurrentZoom;
    
    FVector2D NewZoom = CurrentZoom;
    
    if (bIsShiftDown)
    {
        // Vertical zoom only
        const float ZoomFactor = WheelDelta > 0 ? 1.1f : 0.9f;
        NewZoom.Y *= ZoomFactor;
        NewZoom.Y = FMath::Clamp(NewZoom.Y, 0.1f, 10.0f);
    }
    else if (bIsCtrlDown)
    {
        // Horizontal zoom only
        const float ZoomFactor = WheelDelta > 0 ? 1.1f : 0.9f;
        NewZoom.X *= ZoomFactor;
        NewZoom.X = FMath::Clamp(NewZoom.X, 0.1f, 10.0f);
    }
    else
    {
        // Plain scroll = vertical pan (scroll through notes)
        const float ScrollAmount = WheelDelta * 30.0f;
        CurrentOffset.Y -= ScrollAmount;
        const FVector2D ClampedOffset = ClampOffset(CurrentOffset, MyGeometry.GetLocalSize());
        Offset.Set(*this, ClampedOffset);
        return FReply::Handled();
    }
    
    // Calculate new offset to keep the same content under the mouse cursor
    // After zoom: ScreenPos = ContentPos * NewZoom - NewOffset
    // We want: LocalMousePos = ContentPosUnderMouse * NewZoom - NewOffset
    // Therefore: NewOffset = ContentPosUnderMouse * NewZoom - LocalMousePos
    const FVector2D NewOffset = ContentPosUnderMouse * NewZoom - LocalMousePos;
    const FVector2D ClampedOffset = ClampOffset(NewOffset, MyGeometry.GetLocalSize());
    
    Zoom.Set(*this, NewZoom);
    Offset.Set(*this, ClampedOffset);
    
    return FReply::Handled();
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
double SMidiPianoroll::TickToPixel(double Tick) const
{
    const EMidiTrackTimeMode CurrentTimeMode = TimeMode.Get();
    
    switch(CurrentTimeMode)
    {
    case EMidiTrackTimeMode::TimeLinear:
        {
            if (!LinkedSongsMap.IsValid())
            {
                // Fallback if no song map available
                return (Tick * 0.5f) * Zoom.Get().X;
            }
            const FSongMaps& SongsMap = *LinkedSongsMap;
            const double Milliseconds = SongsMap.TickToMs(Tick);
            return Milliseconds * Zoom.Get().X;
        }

    case EMidiTrackTimeMode::TickLinear:
    default:
        return (Tick * 0.5f) * Zoom.Get().X;
    }
}

double SMidiPianoroll::PixelToTick(double Pixel) const
{
    const EMidiTrackTimeMode CurrentTimeMode = TimeMode.Get();
    const FVector2D LocalOffset = Offset.Get();
    const FVector2D LocalZoom = Zoom.Get();
    
    // Add offset back to get content-space pixel
    const double ContentPixel = Pixel + LocalOffset.X;
    
    switch(CurrentTimeMode)
    {
    case EMidiTrackTimeMode::TimeLinear:
        {
            if (!LinkedSongsMap.IsValid())
            {
                return (ContentPixel / LocalZoom.X) * 2.0;
            }
            const FSongMaps& SongsMap = *LinkedSongsMap;
            const double Milliseconds = ContentPixel / LocalZoom.X;
            return SongsMap.MsToTick(Milliseconds);
        }

    case EMidiTrackTimeMode::TickLinear:
    default:
        return (ContentPixel / LocalZoom.X) * 2.0;
    }
}

void SMidiPianoroll::RecalculateGrid(const FGeometry& AllottedGeometry) const
{
    GridPoints.Empty();
    
    const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
    const FVector2D LocalZoom = Zoom.Get();
    const FVector2D LocalOffset = Offset.Get();
    
    // Default values for when no song map is available
    int32 TicksPerBar = 1920; // Standard 4/4 at 480 PPQ
    int32 TicksPerBeat = 480;
    
    if (LinkedSongsMap.IsValid())
    {
        const FSongMaps& SongsMap = *LinkedSongsMap;
        TicksPerBar = SongsMap.SubdivisionToMidiTicks(EMidiClockSubdivisionQuantization::Bar, 0);
        TicksPerBeat = SongsMap.SubdivisionToMidiTicks(EMidiClockSubdivisionQuantization::Beat, 0);
    }
    
    // Calculate visible tick range
    const double VisibleStartTick = FMath::Max(0.0, PixelToTick(0.0));
    const double VisibleEndTick = PixelToTick(LocalSize.X) + TicksPerBar; // Buffer of one bar
    
    // Calculate pixels per bar for density
    const double PixelsPerBar = TickToPixel(TicksPerBar) - TickToPixel(0);
    CurrentGridDensity = CalculateOptimalGridDensity(PixelsPerBar);
    
    // Start from tick 0 and find visible bars
    int32 DisplayBarNumber = 1;
    double BarTick = 0.0;
    
    // Skip to first visible bar
    while (BarTick + TicksPerBar < VisibleStartTick && DisplayBarNumber <= 500)
    {
        BarTick += TicksPerBar;
        DisplayBarNumber++;
    }
    
    // Add grid points for visible bars
    while (BarTick <= VisibleEndTick && DisplayBarNumber <= 500)
    {
        if (ShouldShowBar(DisplayBarNumber, CurrentGridDensity))
        {
            FPianorollGridPoint GridPoint;
            GridPoint.Type = EPianorollGridPointType::Bar;
            GridPoint.Bar = DisplayBarNumber;
            GridPoint.Beat = 1;
            GridPoint.Subdivision = 1;
            
            GridPoints.Add(static_cast<int32>(BarTick), GridPoint);
        }
        
        // Move to next bar
        BarTick += TicksPerBar;
        DisplayBarNumber++;
    }
}

EPianorollGridDensity SMidiPianoroll::CalculateOptimalGridDensity(float PixelsPerBar) const
{
    if (PixelsPerBar >= 400.0f)
    {
        return EPianorollGridDensity::Subdivisions;
    }
    else if (PixelsPerBar >= 150.0f)
    {
        return EPianorollGridDensity::Beats;
    }
    else if (PixelsPerBar >= MinPixelsPerBarText)
    {
        return EPianorollGridDensity::Bars;
    }
    else if (PixelsPerBar >= MinPixelsPerBarText / 2.0f)
    {
        return EPianorollGridDensity::SparseBars;
    }
    else
    {
        return EPianorollGridDensity::VerySparseBars;
    }
}

bool SMidiPianoroll::ShouldShowBar(int32 BarNumber, EPianorollGridDensity Density) const
{
    switch (Density)
    {
    case EPianorollGridDensity::Subdivisions:
    case EPianorollGridDensity::Beats:
    case EPianorollGridDensity::Bars:
        return true;
        
    case EPianorollGridDensity::SparseBars:
        return (BarNumber % 2) == 1; // Show odd bars (1, 3, 5...)
        
    case EPianorollGridDensity::VerySparseBars:
        return (BarNumber % 4) == 1; // Show every 4th bar (1, 5, 9...)
        
    default:
        return true;
    }
}

int32 SMidiPianoroll::PaintTimeline(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    if (TimelineHeight <= 0.0f)
    {
        return LayerId;
    }
    
    const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
    const FVector2D LocalOffset = Offset.Get();
    
    // Draw timeline background
    FSlateDrawElement::MakeBox(
        OutDrawElements,
        LayerId++,
        AllottedGeometry.ToPaintGeometry(
            FVector2D(LocalSize.X, TimelineHeight),
            FSlateLayoutTransform(FVector2D(0.0f, 0.0f))
        ),
        FAppStyle::GetBrush("Graph.Panel.SolidBackground"),
        ESlateDrawEffect::None,
        FLinearColor(0.02f, 0.02f, 0.02f, 1.0f)
    );
    
    // Draw bar numbers
    const FSlateFontInfo BarFont = FCoreStyle::GetDefaultFontStyle("Regular", 12);
    const FLinearColor BarTextColor = FLinearColor::Gray;
    
    for (const auto& GridPointPair : GridPoints)
    {
        const int32 Tick = GridPointPair.Key;
        const FPianorollGridPoint& GridPoint = GridPointPair.Value;
        
        if (GridPoint.Type == EPianorollGridPointType::Bar)
        {
            const float PixelX = TickToPixel(Tick) - LocalOffset.X;
            
            // Skip if outside visible area
            if (PixelX < -50.0f || PixelX > LocalSize.X + 50.0f)
            {
                continue;
            }
            
            // Draw bar number text
            const FText BarText = FText::AsNumber(GridPoint.Bar);
            
            FSlateDrawElement::MakeText(
                OutDrawElements,
                LayerId,
                AllottedGeometry.ToPaintGeometry(
                    FVector2D(50.0f, TimelineHeight),
                    FSlateLayoutTransform(FVector2D(PixelX + 4.0f, 4.0f))
                ),
                BarText,
                BarFont,
                ESlateDrawEffect::None,
                BarTextColor
            );
        }
    }
    
    return LayerId + 1;
}

int32 SMidiPianoroll::PaintGridLines(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
    const FVector2D LocalOffset = Offset.Get();
    
    const FLinearColor BarLineColor = FLinearColor::Gray.CopyWithNewOpacity(0.3f);
    const FLinearColor BeatLineColor = FLinearColor::Gray.CopyWithNewOpacity(0.15f);
    
    for (const auto& GridPointPair : GridPoints)
    {
        const int32 Tick = GridPointPair.Key;
        const FPianorollGridPoint& GridPoint = GridPointPair.Value;
        
        const float PixelX = TickToPixel(Tick) - LocalOffset.X;
        
        // Skip if outside visible area
        if (PixelX < 0.0f || PixelX > LocalSize.X)
        {
            continue;
        }
        
        FLinearColor LineColor;
        switch (GridPoint.Type)
        {
        case EPianorollGridPointType::Bar:
            LineColor = BarLineColor;
            break;
        case EPianorollGridPointType::Beat:
            LineColor = BeatLineColor;
            break;
        default:
            LineColor = BeatLineColor.CopyWithNewOpacity(0.08f);
            break;
        }
        
        // Draw vertical line from below timeline to bottom
        FSlateDrawElement::MakeLines(
            OutDrawElements,
            LayerId,
            AllottedGeometry.ToPaintGeometry(),
            {
                FVector2D(PixelX, TimelineHeight),
                FVector2D(PixelX, LocalSize.Y)
            },
            ESlateDrawEffect::None,
            LineColor,
            false,
            1.0f
        );
    }
    
    return LayerId + 1;
}

float SMidiPianoroll::GetContentHeight() const
{
    const FVector2D LocalZoom = Zoom.Get();
    const float RowH = 10.0f * LocalZoom.Y;
    // 128 MIDI notes, each with height + 2px gap
    return 128.0f * (RowH + 2.0f);
}

float SMidiPianoroll::GetContentWidth() const
{
    if (!LinkedMidiData.IsValid())
    {
        // Default width if no MIDI data
        return 10000.0f;
    }
    
    // Find the last note end tick
    int32 LastTick = 0;
    for (const FMidiNotesTrack& Track : LinkedMidiData->Tracks)
    {
        for (const FLinkedMidiNote& Note : Track.Notes)
        {
            LastTick = FMath::Max(LastTick, Note.NoteOffTick);
        }
    }
    
    // Add some buffer (one bar worth)
    const int32 TicksPerBar = LinkedSongsMap.IsValid() 
        ? LinkedSongsMap->SubdivisionToMidiTicks(EMidiClockSubdivisionQuantization::Bar, 0) 
        : 1920;
    
    return TickToPixel(LastTick + TicksPerBar);
}

FVector2D SMidiPianoroll::ClampOffset(const FVector2D& InOffset, const FVector2D& ViewportSize) const
{
    const float ContentHeight = GetContentHeight();
    const float ContentWidth = GetContentWidth();
    const float ContentAreaHeight = ViewportSize.Y - TimelineHeight;
    
    FVector2D ClampedOffset = InOffset;
    
    // Horizontal clamping
    // Don't allow panning left of content start (negative offset beyond 0)
    // Don't allow panning right beyond content end
    const float MinOffsetX = 0.0f;
    const float MaxOffsetX = FMath::Max(0.0f, ContentWidth - ViewportSize.X);
    ClampedOffset.X = FMath::Clamp(ClampedOffset.X, MinOffsetX, MaxOffsetX);
    
    // Vertical clamping
    // Don't allow panning above content (offset less than 0 means seeing above note 127)
    // Don't allow panning below content (offset more than content height - viewport height)
    const float MinOffsetY = 0.0f;
    const float MaxOffsetY = FMath::Max(0.0f, ContentHeight - ContentAreaHeight);
    ClampedOffset.Y = FMath::Clamp(ClampedOffset.Y, MinOffsetY, MaxOffsetY);
    
    return ClampedOffset;
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
