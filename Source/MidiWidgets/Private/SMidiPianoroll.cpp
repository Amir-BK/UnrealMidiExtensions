// Fill out your copyright notice in the Description page of Project Settings.


#include "SMidiPianoroll.h"
#include "SlateOptMacros.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Styling/AppStyle.h"
#include "MidiFile/MutableMidiFile.h"


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
	SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION_WITH_NAME(AttributeInitializer, "GridPointType", GridPointType, EInvalidateWidgetReason::Paint);
	SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION_WITH_NAME(AttributeInitializer, "EditMode", EditMode, EInvalidateWidgetReason::Paint);
	SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION_WITH_NAME(AttributeInitializer, "EditingTrackIndex", EditingTrackIndex, EInvalidateWidgetReason::Paint);
	SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION_WITH_NAME(AttributeInitializer, "DefaultNoteVelocity", DefaultNoteVelocity, EInvalidateWidgetReason::None);
	SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION_WITH_NAME(AttributeInitializer, "DefaultNoteDurationTicks", DefaultNoteDurationTicks, EInvalidateWidgetReason::None);
	SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION_WITH_NAME(AttributeInitializer, "GridSubdivision", GridSubdivision, EInvalidateWidgetReason::Paint);
	SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION_WITH_NAME(AttributeInitializer, "NoteSnapping", NoteSnapping, EInvalidateWidgetReason::None);
	SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION_WITH_NAME(AttributeInitializer, "NoteDuration", NoteDuration, EInvalidateWidgetReason::None);
}

SMidiPianoroll::SMidiPianoroll()
	: Offset(*this, FVector2D::ZeroVector)
	, Zoom(*this, FVector2D(1.0f, 1.0f))
	, VisualizationData(*this, FMidiFileVisualizationData())
	, TimeMode(*this, EMidiTrackTimeMode::TimeLinear)
	, GridPointType(*this, EPianorollGridPointType::Bar)
	, EditMode(*this, EPianorollEditMode::Select)
	, EditingTrackIndex(*this, 0)
	, DefaultNoteVelocity(*this, 100)
	, DefaultNoteDurationTicks(*this, 480)
	, GridSubdivision(*this, EMidiClockSubdivisionQuantization::SixteenthNote)
	, NoteSnapping(*this, EMidiClockSubdivisionQuantization::SixteenthNote)
	, NoteDuration(*this, EMidiClockSubdivisionQuantization::SixteenthNote)
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
	bIsEditable = InArgs._bIsEditable;

	Offset.Assign(*this, InArgs._Offset);
	Zoom.Assign(*this, InArgs._Zoom);
	VisualizationData.Assign(*this, InArgs._VisualizationData);
	TimeMode.Assign(*this, InArgs._TimeMode);
	GridPointType.Assign(*this, InArgs._GridPointType);
	EditMode.Assign(*this, InArgs._EditMode);
	EditingTrackIndex.Assign(*this, InArgs._EditingTrackIndex);
	DefaultNoteVelocity.Assign(*this, InArgs._DefaultNoteVelocity);
	DefaultNoteDurationTicks.Assign(*this, InArgs._DefaultNoteDurationTicks);
	GridSubdivision.Assign(*this, InArgs._GridSubdivision);
	NoteSnapping.Assign(*this, InArgs._NoteSnapping);
	NoteDuration.Assign(*this, InArgs._NoteDuration);

	ChildSlot
	[
		SNullWidget::NullWidget
	];
}
int32 SMidiPianoroll::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    const FSlateBrush* Brush = &PianorollStyle->GridBrush;
	const FSlateBrush* NoteBrush = &PianorollStyle->NoteBrush;
	const FSlateBrush* SelectedNoteBrush = &PianorollStyle->SelectedNoteBrush;

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

            for (int32 NoteIdx = 0; NoteIdx < Track.Notes.Num(); ++NoteIdx)
            {
                const FLinkedMidiNote& Note = Track.Notes[NoteIdx];
                const float X = TickToPixel(Note.NoteOnTick) - LocalOffset.X;
                const float EndX = TickToPixel(Note.NoteOffTick) - LocalOffset.X;
                const float W = FMath::Max(EndX - X, 1.0f);
                const float RowH = 10.0f * LocalZoom.Y;
                const float Y = ContentStartY + (127 - Note.NoteNumber) * (RowH + 2.0f) - LocalOffset.Y;

                if (X > LocalSize.X || X + W < 0.0f || Y > LocalSize.Y || Y + RowH < TimelineHeight)
                {
                    continue;
                }

                // Check if note is selected
                const bool bIsSelected = IsNoteSelected(TrackIdx, NoteIdx);
                const FSlateBrush* CurrentNoteBrush = bIsSelected ? SelectedNoteBrush : NoteBrush;
               // FLinearColor NoteColor = bIsSelected ? FLinearColor::White : TrackColor;

                // Use TrackIdx for layer ordering to ensure consistent z-order
                FSlateDrawElement::MakeBox(
                    OutDrawElements,
                    LayerId + TrackIdx,
                    AllottedGeometry.ToPaintGeometry(FVector2D(W, RowH), FSlateLayoutTransform(FVector2D(X, Y))),
                    CurrentNoteBrush,
                    ESlateDrawEffect::None,
                    TrackColor);
            }
        }
        LayerId += LinkedMidiData->Tracks.Num();
    }
    LayerId++;

    // Layer 4: Draw marquee selection rectangle
    if (bIsMarqueeSelecting)
    {
        const FVector2D MarqueeMin(
            FMath::Min(MarqueeStartPos.X, MarqueeCurrentPos.X),
            FMath::Min(MarqueeStartPos.Y, MarqueeCurrentPos.Y)
        );
        const FVector2D MarqueeSize(
            FMath::Abs(MarqueeCurrentPos.X - MarqueeStartPos.X),
            FMath::Abs(MarqueeCurrentPos.Y - MarqueeStartPos.Y)
        );
        
        // Draw filled semi-transparent rectangle
        FSlateDrawElement::MakeBox(
            OutDrawElements,
            LayerId++,
            AllottedGeometry.ToPaintGeometry(MarqueeSize, FSlateLayoutTransform(MarqueeMin)),
            FAppStyle::GetBrush("MarqueeSelection"),
            ESlateDrawEffect::None,
            FLinearColor(0.2f, 0.5f, 1.0f, 0.3f)
        );
        
        // Draw border
        TArray<FVector2D> Lines = {
            MarqueeMin,
            FVector2D(MarqueeMin.X + MarqueeSize.X, MarqueeMin.Y),
            FVector2D(MarqueeMin.X + MarqueeSize.X, MarqueeMin.Y + MarqueeSize.Y),
            FVector2D(MarqueeMin.X, MarqueeMin.Y + MarqueeSize.Y),
            MarqueeMin
        };
        
        FSlateDrawElement::MakeLines(
            OutDrawElements,
            LayerId++,
            AllottedGeometry.ToPaintGeometry(),
            Lines,
            ESlateDrawEffect::None,
            FLinearColor(0.2f, 0.5f, 1.0f, 0.8f),
            true,
            2.0f
        );
    }

    // Layer 5: Draw preview note when in paint mode
    if (bShowPreviewNote && bIsEditable && EditMode.Get() == EPianorollEditMode::Paint && LinkedMidiData.IsValid())
    {
        const int32 TargetTrackIndex = EditingTrackIndex.Get();
        if (TargetTrackIndex >= 0 && TargetTrackIndex < LinkedMidiData->Tracks.Num())
        {
            // Get track color for preview
            const FMidiFileVisualizationData& VisData = VisualizationData.Get();
            const FMidiNotesTrack& Track = LinkedMidiData->Tracks[TargetTrackIndex];
            FLinearColor PreviewColor = FLinearColor::White;
            
            int32 IndexOfTrackVis = VisData.TrackVisualizations.IndexOfByPredicate([&Track](const FMidiTrackVisualizationData& Data)
            {
                return Data.TrackIndex == Track.TrackIndex && Data.ChannelIndex == Track.ChannelIndex;
            });
            if (IndexOfTrackVis == INDEX_NONE)
            {
                IndexOfTrackVis = VisData.TrackVisualizations.IndexOfByPredicate([&Track](const FMidiTrackVisualizationData& Data)
                {
                    return Data.TrackIndex == Track.TrackIndex;
                });
            }
            if (IndexOfTrackVis != INDEX_NONE)
            {
                PreviewColor = VisData.TrackVisualizations[IndexOfTrackVis].TrackColor;
            }
            
            // Make preview semi-transparent
            PreviewColor.A = 0.4f;
            
            // Calculate preview note duration
            int32 PreviewDurationTicks = DefaultNoteDurationTicks.Get();
            if (LinkedSongsMap.IsValid())
            {
                PreviewDurationTicks = LinkedSongsMap->SubdivisionToMidiTicks(NoteDuration.Get(), 0);
            }
            
            const float ContentStartY = TimelineHeight;
            const float RowH = 10.0f * LocalZoom.Y;
            const float PreviewX = TickToPixel(PreviewNoteTick) - LocalOffset.X;
            const float PreviewEndX = TickToPixel(PreviewNoteTick + PreviewDurationTicks) - LocalOffset.X;
            const float PreviewW = FMath::Max(PreviewEndX - PreviewX, 1.0f);
            const float PreviewY = ContentStartY + (127 - PreviewNoteNumber) * (RowH + 2.0f) - LocalOffset.Y;
            
            // Only draw if visible
            if (PreviewX <= LocalSize.X && PreviewX + PreviewW >= 0.0f && 
                PreviewY <= LocalSize.Y && PreviewY + RowH >= TimelineHeight)
            {
                FSlateDrawElement::MakeBox(
                    OutDrawElements,
                    LayerId++,
                    AllottedGeometry.ToPaintGeometry(FVector2D(PreviewW, RowH), FSlateLayoutTransform(FVector2D(PreviewX, PreviewY))),
                    NoteBrush,
                    ESlateDrawEffect::None,
                    PreviewColor
                );
            }
        }
    }

    // Layer 6: Paint timeline header (on top of everything)
    LayerId = PaintTimeline(AllottedGeometry, OutDrawElements, LayerId);
 
    return LayerId;
}
FReply SMidiPianoroll::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    const FVector2D LocalMousePos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
    
    // Update preview note state for paint mode
    if (bIsEditable && EditMode.Get() == EPianorollEditMode::Paint && !bIsPainting)
    {
        LastMousePos = LocalMousePos;
        PreviewNoteTick = SnapTickToGrid(static_cast<int32>(PixelToTick(LocalMousePos.X)));
        PreviewNoteNumber = ScreenYToNoteNumber(LocalMousePos.Y, MyGeometry);
        bShowPreviewNote = (LocalMousePos.Y >= TimelineHeight);
    }
    else
    {
        bShowPreviewNote = false;
    }
    
    // Handle note dragging
    if (bIsDraggingNotes && bIsEditable)
    {
        // Calculate the delta in ticks and note numbers
        const int32 CurrentTick = SnapTickToGrid(static_cast<int32>(PixelToTick(LocalMousePos.X)));
        const int32 CurrentNoteNumber = ScreenYToNoteNumber(LocalMousePos.Y, MyGeometry);
        
        const int32 DeltaTicks = CurrentTick - DragStartTick;
        const int32 DeltaNotes = CurrentNoteNumber - DragStartNoteNumber;
        
        // Only update if there's actual movement
        if (DeltaTicks != 0 || DeltaNotes != 0)
        {
            // Build edit operations for all selected notes
            TArray<FNotesEditCallbackData> Edits;
            
            for (const FNoteIdentifier& NoteId : SelectedNotes)
            {
                if (LinkedMidiData.IsValid() && 
                    LinkedMidiData->Tracks.IsValidIndex(NoteId.TrackIndex) &&
                    LinkedMidiData->Tracks[NoteId.TrackIndex].Notes.IsValidIndex(NoteId.NoteIndex))
                {
                    const FLinkedMidiNote& OriginalNote = LinkedMidiData->Tracks[NoteId.TrackIndex].Notes[NoteId.NoteIndex];
                    
                    FNotesEditCallbackData Edit;
                    Edit.TrackIndex = NoteId.TrackIndex;
                    Edit.NoteIndex = NoteId.NoteIndex;
                    Edit.NoteData = OriginalNote;
                    Edit.NoteData.NoteOnTick = FMath::Max(0, OriginalNote.NoteOnTick + DeltaTicks);
                    Edit.NoteData.NoteOffTick = FMath::Max(Edit.NoteData.NoteOnTick + 1, OriginalNote.NoteOffTick + DeltaTicks);
                    Edit.NoteData.NoteNumber = FMath::Clamp(OriginalNote.NoteNumber + DeltaNotes, 0, 127);
                    Edit.bDelete = false;
                    Edits.Add(Edit);
                }
            }
            
            if (Edits.Num() > 0 && OnNotesModified.IsBound())
            {
                OnNotesModified.Execute(Edits);
                // Update drag start to current position for continuous dragging
                DragStartTick = CurrentTick;
                DragStartNoteNumber = CurrentNoteNumber;
            }
        }
        
        return FReply::Handled();
    }
    
    if (bIsMarqueeSelecting)
    {
        MarqueeCurrentPos = LocalMousePos;
        // Force repaint to show marquee
        return FReply::Handled();
    }
    
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
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bIsEditable)
    {
        const FVector2D LocalMousePos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
        const EPianorollEditMode CurrentEditMode = EditMode.Get();
        
        // Check if we're in Paint mode
        if (CurrentEditMode == EPianorollEditMode::Paint)
        {
            // Paint a new note at the click position
            const int32 ClickTick = SnapTickToGrid(static_cast<int32>(PixelToTick(LocalMousePos.X)));
            const int32 ClickNoteNumber = ScreenYToNoteNumber(LocalMousePos.Y, MyGeometry);
            
            // Get the track to paint on
            const int32 TargetTrackIndex = EditingTrackIndex.Get();
            
            if (LinkedMidiData.IsValid() && TargetTrackIndex >= 0 && TargetTrackIndex < LinkedMidiData->Tracks.Num())
            {
                // Calculate note duration from NoteDuration
                int32 NoteDurationTicks = DefaultNoteDurationTicks.Get();
                if (LinkedSongsMap.IsValid())
                {
                    NoteDurationTicks = LinkedSongsMap->SubdivisionToMidiTicks(NoteDuration.Get(), 0);
                }
                
                // Create a new note
                FLinkedMidiNote NewNote;
                NewNote.NoteOnTick = ClickTick;
                NewNote.NoteOffTick = ClickTick + NoteDurationTicks;
                NewNote.NoteNumber = ClickNoteNumber;
                NewNote.Velocity = DefaultNoteVelocity.Get();
                
                // Create an edit to add the note (use an invalid index to signal addition)
                FNotesEditCallbackData Edit;
                Edit.TrackIndex = TargetTrackIndex;
                Edit.NoteIndex = LinkedMidiData->Tracks[TargetTrackIndex].Notes.Num(); // Add at end
                Edit.NoteData = NewNote;
                Edit.bDelete = false;
                
                TArray<FNotesEditCallbackData> Edits;
                Edits.Add(Edit);
                
                if (OnNotesModified.IsBound())
                {
                    OnNotesModified.Execute(Edits);
                }
                
                bIsPainting = true;
                return FReply::Handled().CaptureMouse(SharedThis(this));
            }
        }
        else // Select mode
        {
            // Check if clicking on an existing note
            int32 ClickedTrackIndex = INDEX_NONE;
            int32 ClickedNoteIndex = INDEX_NONE;
            
            if (FindNoteAtPosition(LocalMousePos, MyGeometry, ClickedTrackIndex, ClickedNoteIndex))
            {
                // Clicked on a note
                FNoteIdentifier ClickedNoteId;
                ClickedNoteId.TrackIndex = ClickedTrackIndex;
                ClickedNoteId.NoteIndex = ClickedNoteIndex;
                
                if (MouseEvent.IsShiftDown())
                {
                    // Toggle selection
                    if (SelectedNotes.Contains(ClickedNoteId))
                    {
                        SelectedNotes.Remove(ClickedNoteId);
                    }
                    else
                    {
                        SelectedNotes.Add(ClickedNoteId);
                    }
                }
                else if (!SelectedNotes.Contains(ClickedNoteId))
                {
                    // Select only this note if not already selected
                    SelectedNotes.Empty();
                    SelectedNotes.Add(ClickedNoteId);
                }
                
                // Start dragging if we have a selection
                if (SelectedNotes.Num() > 0)
                {
                    bIsDraggingNotes = true;
                    DragStartPos = LocalMousePos;
                    DragStartTick = SnapTickToGrid(static_cast<int32>(PixelToTick(LocalMousePos.X)));
                    DragStartNoteNumber = ScreenYToNoteNumber(LocalMousePos.Y, MyGeometry);
                    return FReply::Handled().CaptureMouse(SharedThis(this));
                }
            }
            else
            {
                // Clicked on empty space - start marquee selection
                bIsMarqueeSelecting = true;
                MarqueeStartPos = LocalMousePos;
                MarqueeCurrentPos = MarqueeStartPos;
                
                // Clear previous selection if not holding Shift
                if (!MouseEvent.IsShiftDown())
                {
                    SelectedNotes.Empty();
                }
                
                return FReply::Handled().CaptureMouse(SharedThis(this));
            }
        }
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
    if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        if (bIsDraggingNotes)
        {
            bIsDraggingNotes = false;
            return FReply::Handled().ReleaseMouseCapture();
        }
        
        if (bIsPainting)
        {
            bIsPainting = false;
            return FReply::Handled().ReleaseMouseCapture();
        }
        
        if (bIsMarqueeSelecting)
        {
            // Calculate selection bounds in content space
            PerformMarqueeSelection(MyGeometry, MouseEvent.IsShiftDown());
            bIsMarqueeSelecting = false;
            return FReply::Handled().ReleaseMouseCapture();
        }
    }
    
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
        NewZoom.X = FMath::Clamp(NewZoom.X, 0.001f, 10.0f);
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
    if (bIsDraggingNotes)
    {
        return EMouseCursor::GrabHandClosed;
    }
    
    if (bIsMarqueeSelecting)
    {
        return EMouseCursor::Crosshairs;
    }
    
    if (bIsPainting)
    {
        return EMouseCursor::Crosshairs;
    }
    
    if (bIsRightMouseButtonDown)
    {
        if (bIsPanning)
        {
            return EMouseCursor::GrabHandClosed;
        }
		return EMouseCursor::GrabHand;
    }
    
    // Show different cursor based on edit mode
    if (bIsEditable)
    {
        const EPianorollEditMode CurrentEditMode = EditMode.Get();
        if (CurrentEditMode == EPianorollEditMode::Paint)
        {
            return EMouseCursor::Crosshairs;
        }
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
    const EPianorollGridPointType CurrentGridType = GridPointType.Get();
    
    // Default values for when no song map is available
    int32 TicksPerBar = 1920; // Standard 4/4 at 480 PPQ
    int32 TicksPerBeat = 480;
    int32 TicksPerSubdivision = 120; // 16th notes
    int32 BeatsPerBar = 4;
    
    if (LinkedSongsMap.IsValid())
    {
        const FSongMaps& SongsMap = *LinkedSongsMap;
        TicksPerBar = SongsMap.SubdivisionToMidiTicks(EMidiClockSubdivisionQuantization::Bar, 0);
        TicksPerBeat = SongsMap.SubdivisionToMidiTicks(EMidiClockSubdivisionQuantization::Beat, 0);
        // Use the configured GridSubdivision for visual grid
        TicksPerSubdivision = SongsMap.SubdivisionToMidiTicks(GridSubdivision.Get(), 0);
        
        // Get time signature to determine beats per bar
        const FTimeSignature* TimeSig = SongsMap.GetTimeSignatureAtTick(0);
        if (TimeSig)
        {
            BeatsPerBar = TimeSig->Numerator;
        }
    }
    
    // Calculate how many subdivisions per beat
    const int32 SubdivisionsPerBeat = (TicksPerSubdivision > 0) ? FMath::Max(1, TicksPerBeat / TicksPerSubdivision) : 4;
    
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
    
    // Add grid points for visible bars and their contents
    while (BarTick <= VisibleEndTick && DisplayBarNumber <= 500)
    {
        if (ShouldShowBar(DisplayBarNumber, CurrentGridDensity))
        {
            // Always add bar grid points
            FPianorollGridPoint BarGridPoint;
            BarGridPoint.Type = EPianorollGridPointType::Bar;
            BarGridPoint.Bar = DisplayBarNumber;
            BarGridPoint.Beat = 1;
            BarGridPoint.Subdivision = 1;
            GridPoints.Add(static_cast<int32>(BarTick), BarGridPoint);
            
            // Add beat grid points if requested
            if (CurrentGridType == EPianorollGridPointType::Beat || 
                (CurrentGridType == EPianorollGridPointType::Subdivision && CurrentGridDensity <= EPianorollGridDensity::Beats))
            {
                for (int32 BeatNum = 2; BeatNum <= BeatsPerBar; ++BeatNum)
                {
                    const double BeatTick = BarTick + (BeatNum - 1) * TicksPerBeat;
                    if (BeatTick <= VisibleEndTick)
                    {
                        FPianorollGridPoint BeatGridPoint;
                        BeatGridPoint.Type = EPianorollGridPointType::Beat;
                        BeatGridPoint.Bar = DisplayBarNumber;
                        BeatGridPoint.Beat = BeatNum;
                        BeatGridPoint.Subdivision = 1;
                        GridPoints.Add(static_cast<int32>(BeatTick), BeatGridPoint);
                    }
                }
            }
            
            // Add subdivision grid points if requested and zoom allows
            if (CurrentGridType == EPianorollGridPointType::Subdivision && 
                CurrentGridDensity == EPianorollGridDensity::Subdivisions)
            {
                for (int32 BeatNum = 1; BeatNum <= BeatsPerBar; ++BeatNum)
                {
                    const double BeatStartTick = BarTick + (BeatNum - 1) * TicksPerBeat;
                    for (int32 SubDiv = 2; SubDiv <= SubdivisionsPerBeat; ++SubDiv)
                    {
                        const double SubDivTick = BeatStartTick + (SubDiv - 1) * TicksPerSubdivision;
                        if (SubDivTick <= VisibleEndTick && SubDivTick < BarTick + TicksPerBar)
                        {
                            FPianorollGridPoint SubDivGridPoint;
                            SubDivGridPoint.Type = EPianorollGridPointType::Subdivision;
                            SubDivGridPoint.Bar = DisplayBarNumber;
                            SubDivGridPoint.Beat = BeatNum;
                            SubDivGridPoint.Subdivision = SubDiv;
                            GridPoints.Add(static_cast<int32>(SubDivTick), SubDivGridPoint);
                        }
                    }
                }
            }
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

void SMidiPianoroll::PerformMarqueeSelection(const FGeometry& MyGeometry, bool bAddToSelection)
{
    if (!LinkedMidiData.IsValid())
    {
        return;
    }
    
    const FVector2D LocalOffset = Offset.Get();
    const FVector2D LocalZoom = Zoom.Get();
    const float ContentStartY = TimelineHeight;
    
    // Calculate marquee bounds (min/max to handle any drag direction)
    const FVector2D MarqueeMin(
        FMath::Min(MarqueeStartPos.X, MarqueeCurrentPos.X),
        FMath::Min(MarqueeStartPos.Y, MarqueeCurrentPos.Y)
    );
    const FVector2D MarqueeMax(
        FMath::Max(MarqueeStartPos.X, MarqueeCurrentPos.X),
        FMath::Max(MarqueeStartPos.Y, MarqueeCurrentPos.Y)
    );
    
    // Iterate through all notes and check intersection
    for (int32 TrackIdx = 0; TrackIdx < LinkedMidiData->Tracks.Num(); ++TrackIdx)
    {
        const FMidiNotesTrack& Track = LinkedMidiData->Tracks[TrackIdx];

		// Need to check if track is visible in visualization data
		const FMidiFileVisualizationData& VisData = VisualizationData.Get();
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
		}

        
        for (int32 NoteIdx = 0; NoteIdx < Track.Notes.Num(); ++NoteIdx)
        {
            const FLinkedMidiNote& Note = Track.Notes[NoteIdx];
            
            // Calculate note bounds in screen space
            const float NoteX = TickToPixel(Note.NoteOnTick) - LocalOffset.X;
            const float NoteEndX = TickToPixel(Note.NoteOffTick) - LocalOffset.X;
            const float RowH = 10.0f * LocalZoom.Y;
            const float NoteY = ContentStartY + (127 - Note.NoteNumber) * (RowH + 2.0f) - LocalOffset.Y;
            const float NoteH = RowH;
            
            // Check intersection with marquee
            const bool bIntersects = 
                NoteX < MarqueeMax.X && 
                NoteEndX > MarqueeMin.X &&
                NoteY < MarqueeMax.Y && 
                NoteY + NoteH > MarqueeMin.Y;
            
            if (bIntersects)
            {
                // Add to selection
                FNoteIdentifier NoteId;
                NoteId.TrackIndex = TrackIdx;
                NoteId.NoteIndex = NoteIdx;
                SelectedNotes.Add(NoteId);
            }
        }
    }
}

bool SMidiPianoroll::IsNoteSelected(int32 TrackIndex, int32 NoteIndex) const
{
    FNoteIdentifier NoteId;
    NoteId.TrackIndex = TrackIndex;
    NoteId.NoteIndex = NoteIndex;
    return SelectedNotes.Contains(NoteId);
}

int32 SMidiPianoroll::ScreenYToNoteNumber(float ScreenY, const FGeometry& AllottedGeometry) const
{
    const FVector2D LocalOffset = Offset.Get();
    const FVector2D LocalZoom = Zoom.Get();
    const float ContentStartY = TimelineHeight;
    const float RowH = 10.0f * LocalZoom.Y;
    const float RowSpacing = RowH + 2.0f;
    
    // Convert screen Y to content Y
    const float ContentY = ScreenY + LocalOffset.Y - ContentStartY;
    
    // Calculate which row we're in
    const int32 RowIndex = FMath::FloorToInt(ContentY / RowSpacing);
    
    // Convert row index to note number (127 - row because notes are drawn top-down)
    const int32 NoteNumber = FMath::Clamp(127 - RowIndex, 0, 127);
    
    return NoteNumber;
}

bool SMidiPianoroll::FindNoteAtPosition(const FVector2D& ScreenPos, const FGeometry& AllottedGeometry, int32& OutTrackIndex, int32& OutNoteIndex) const
{
    if (!LinkedMidiData.IsValid())
    {
        return false;
    }
    
    const FVector2D LocalOffset = Offset.Get();
    const FVector2D LocalZoom = Zoom.Get();
    const float ContentStartY = TimelineHeight;
    const float RowH = 10.0f * LocalZoom.Y;
    
    const FMidiFileVisualizationData& VisData = VisualizationData.Get();
    
    // Search through all tracks and notes
    for (int32 TrackIdx = 0; TrackIdx < LinkedMidiData->Tracks.Num(); ++TrackIdx)
    {
        const FMidiNotesTrack& Track = LinkedMidiData->Tracks[TrackIdx];
        
        // Check track visibility
        int32 IndexOfTrackVis = VisData.TrackVisualizations.IndexOfByPredicate([&Track](const FMidiTrackVisualizationData& Data)
        {
            return Data.TrackIndex == Track.TrackIndex && Data.ChannelIndex == Track.ChannelIndex;
        });
        
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
            
        if (Vis && !Vis->bIsVisible)
        {
            continue;
        }
        
        for (int32 NoteIdx = 0; NoteIdx < Track.Notes.Num(); ++NoteIdx)
        {
            const FLinkedMidiNote& Note = Track.Notes[NoteIdx];
            
            const float NoteX = TickToPixel(Note.NoteOnTick) - LocalOffset.X;
            const float NoteEndX = TickToPixel(Note.NoteOffTick) - LocalOffset.X;
            const float NoteY = ContentStartY + (127 - Note.NoteNumber) * (RowH + 2.0f) - LocalOffset.Y;
            
            if (ScreenPos.X >= NoteX && ScreenPos.X <= NoteEndX &&
                ScreenPos.Y >= NoteY && ScreenPos.Y <= NoteY + RowH)
            {
                OutTrackIndex = TrackIdx;
                OutNoteIndex = NoteIdx;
                return true;
            }
        }
    }
    
    return false;
}

int32 SMidiPianoroll::SnapTickToGrid(int32 Tick) const
{
    if (!LinkedSongsMap.IsValid())
    {
        // Default snap to 16th notes at 480 PPQ
        const int32 SnapInterval = 120;
        return FMath::RoundToInt((float)Tick / SnapInterval) * SnapInterval;
    }
    
    // Use NoteSnapping for the snap interval when editing
    const int32 SnapInterval = LinkedSongsMap->SubdivisionToMidiTicks(NoteSnapping.Get(), 0);
    
    return FMath::RoundToInt((float)Tick / SnapInterval) * SnapInterval;
}

FReply SMidiPianoroll::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
    if (!bIsEditable)
    {
        return FReply::Unhandled();
    }
    
    // Handle Delete key to delete selected notes
    if (InKeyEvent.GetKey() == EKeys::Delete || InKeyEvent.GetKey() == EKeys::BackSpace)
    {
        if (SelectedNotes.Num() > 0 && OnDeleteSelectedNotes.IsBound())
        {
            OnDeleteSelectedNotes.Execute();
            return FReply::Handled();
        }
    }
    
    // Handle Escape to clear selection
    if (InKeyEvent.GetKey() == EKeys::Escape)
    {
        SelectedNotes.Empty();
        return FReply::Handled();
    }
    
    // Handle Ctrl+A to select all visible notes
    if (InKeyEvent.GetKey() == EKeys::A && InKeyEvent.IsControlDown())
    {
        if (LinkedMidiData.IsValid())
        {
            SelectedNotes.Empty();
            const FMidiFileVisualizationData& VisData = VisualizationData.Get();
            
            for (int32 TrackIdx = 0; TrackIdx < LinkedMidiData->Tracks.Num(); ++TrackIdx)
            {
                const FMidiNotesTrack& Track = LinkedMidiData->Tracks[TrackIdx];
                
                // Check visibility
                int32 IndexOfTrackVis = VisData.TrackVisualizations.IndexOfByPredicate([&Track](const FMidiTrackVisualizationData& Data)
                {
                    return Data.TrackIndex == Track.TrackIndex && Data.ChannelIndex == Track.ChannelIndex;
                });
                
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
                    
                if (Vis && !Vis->bIsVisible)
                {
                    continue;
                }
                
                for (int32 NoteIdx = 0; NoteIdx < Track.Notes.Num(); ++NoteIdx)
                {
                    FNoteIdentifier NoteId;
                    NoteId.TrackIndex = TrackIdx;
                    NoteId.NoteIndex = NoteIdx;
                    SelectedNotes.Add(NoteId);
                }
            }
            return FReply::Handled();
        }
    }
    
    return FReply::Unhandled();
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
