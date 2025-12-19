// Copyright notice in the Description page of Project Settings.

#include "MidiPianoroll.h"
#include "HarmonixMidi/MidiFile.h"
#include "MidiFile/MidiNotesData.h"
#include "MidiFile/MutableMidiFile.h"


void UMidiPianoroll::SetMidiFile(UMidiFile* InMidiFile)
{
    LinkedMidiFile = InMidiFile;
    
    // Auto-enable editable mode if the file is a MutableMidiFile
	const bool bIsMutable = LinkedMidiFile->IsA<UMutableMidiFile>();
    
    if (PianorollWidget.IsValid())
    {
        if (LinkedMidiFile)
        {
            TSharedPtr<FSongMaps, ESPMode::ThreadSafe> SongsMap = MakeShared<FSongMaps, ESPMode::ThreadSafe>(*LinkedMidiFile->GetSongMaps());
			auto MidiData = FMidiNotesData::BuildFromMidiFile(LinkedMidiFile);
            PianorollWidget->SetMidiData(MidiData, SongsMap);
            VisualizationData = FMidiFileVisualizationData::BuildFromLinkedMidiData(*MidiData);
            PianorollWidget->SetIsEditable(bIsMutable);
        }
        else
        {
            PianorollWidget->SetMidiData(nullptr, nullptr);
            VisualizationData = FMidiFileVisualizationData();
            PianorollWidget->SetIsEditable(false);
        }
    }
}

void UMidiPianoroll::MakeEditableCopyOfLinkedMidiFile()
{
    if (LinkedMidiFile)
    {
        UMutableMidiFile* MutableCopy = NewObject<UMutableMidiFile>(this, UMutableMidiFile::StaticClass());
        MutableCopy->InitializeFromMidiFile(LinkedMidiFile);
		// Name the mutable copy based on the original file's name with a prefix indicating it's editable
		FString OriginalName = LinkedMidiFile->GetName();
		MutableCopy->Rename(*(FString::Printf(TEXT("Editable_%s"), *OriginalName)), this);
		SetMidiFile(MutableCopy);
		// SetMidiFile will auto-enable editable since it's now a MutableMidiFile
    }
}

void UMidiPianoroll::SaveMidiFileToAsset()
{
	SaveMidiFileAsAsset(TEXT("/Game/MIDI"), LinkedMidiFile ? LinkedMidiFile->GetName() + TEXT("_Saved") : TEXT("NewMidiFile"));
}

void UMidiPianoroll::SetEditingTrackIndex(int32 InTrackIndex)
{
    EditingTrackIndex = InTrackIndex;
    // The Slate widget will pick this up via the TAttribute binding
}

void UMidiPianoroll::SetEditMode(EPianorollEditMode InEditMode)
{
    EditMode = InEditMode;
    // The Slate widget will pick this up via the TAttribute binding
}

void UMidiPianoroll::SetDefaultNoteVelocity(int32 InVelocity)
{
    DefaultNoteVelocity = FMath::Clamp(InVelocity, 1, 127);
    // The Slate widget will pick this up via the TAttribute binding
}

void UMidiPianoroll::SetDefaultNoteDurationTicks(int32 InDurationTicks)
{
    DefaultNoteDurationTicks = FMath::Max(1, InDurationTicks);
    // The Slate widget will pick this up via the TAttribute binding
}

void UMidiPianoroll::DeleteSelectedNotes()
{
    if (!PianorollWidget.IsValid() || !LinkedMidiFile)
    {
        return;
    }

    UMutableMidiFile* MutableFile = Cast<UMutableMidiFile>(LinkedMidiFile);
    if (!MutableFile)
    {
        UE_LOG(LogTemp, Warning, TEXT("DeleteSelectedNotes: LinkedMidiFile is not a MutableMidiFile"));
        return;
    }

    const auto& SelectedNotes = PianorollWidget->GetSelectedNotes();
    if (SelectedNotes.Num() == 0)
    {
        return;
    }

    // Build delete edits for all selected notes
    TArray<FNotesEditCallbackData> Edits;
    for (const auto& NoteId : SelectedNotes)
    {
        FNotesEditCallbackData Edit;
        Edit.TrackIndex = NoteId.TrackIndex;
        Edit.NoteIndex = NoteId.NoteIndex;
        Edit.bDelete = true;
        Edits.Add(Edit);
    }

    // Clear selection before modifying
    PianorollWidget->ClearSelection();

    // Apply the deletions
    MutableFile->ModifyNotes(Edits);

    // Refresh the display
    SetMidiFile(LinkedMidiFile);
}

UMidiFile* UMidiPianoroll::SaveMidiFileAsAsset(const FString& PackagePath, const FString& AssetName)
{
    UMutableMidiFile* MutableFile = Cast<UMutableMidiFile>(LinkedMidiFile);
    if (!MutableFile)
    {
        UE_LOG(LogTemp, Warning, TEXT("SaveMidiFileAsAsset: LinkedMidiFile is not a MutableMidiFile"));
        return nullptr;
    }

    return MutableFile->SaveAsAsset(PackagePath, AssetName);
}

void UMidiPianoroll::ClearSelection()
{
    if (PianorollWidget.IsValid())
    {
        PianorollWidget->ClearSelection();
    }
}

int32 UMidiPianoroll::GetSelectedNoteCount() const
{
    if (PianorollWidget.IsValid())
    {
        return PianorollWidget->GetSelectedNotes().Num();
    }
    return 0;
}

bool UMidiPianoroll::IsEditable() const
{
    return Cast<UMutableMidiFile>(LinkedMidiFile) != nullptr;
}

TSharedRef<SWidget> UMidiPianoroll::RebuildWidget()
{
    TSharedPtr<FMidiNotesData, ESPMode::ThreadSafe> MidiData;
    TSharedPtr<FSongMaps, ESPMode::ThreadSafe> SongsMap;

    // Auto-enable editable mode if the file is a MutableMidiFile
    bool bIsMutable = Cast<UMutableMidiFile>(LinkedMidiFile) != nullptr;

    if(LinkedMidiFile)
    {
        MidiData = FMidiNotesData::BuildFromMidiFile(LinkedMidiFile);
        SongsMap = MakeShared<FSongMaps, ESPMode::ThreadSafe>(*LinkedMidiFile->GetSongMaps());
        VisualizationData = FMidiFileVisualizationData::BuildFromLinkedMidiData(*MidiData);
    }

    SAssignNew(PianorollWidget, SMidiPianoroll)
        .Clipping(EWidgetClipping::ClipToBounds)
        .LinkedMidiData(MidiData)
        .LinkedSongsMap(SongsMap)
        // Bind to getter using lambda - will be called each frame during paint
        .VisualizationData(TAttribute<FMidiFileVisualizationData>::CreateLambda([this]() { return GetVisualizationData(); }))
        .TimeMode(TAttribute<EMidiTrackTimeMode>::CreateLambda([this]() { return GetTimeDisplayMode(); }))
        .PianorollStyle(&PianorollStyle)
        .Offset(FVector2D::ZeroVector)
        .Zoom(FVector2D(1.0f, 1.0f))
        .GridPointType(TAttribute<EPianorollGridPointType>::CreateLambda([this]() { return GetGridPointType(); }))
        .EditMode(TAttribute<EPianorollEditMode>::CreateLambda([this]() { return GetEditMode(); }))
        .EditingTrackIndex(TAttribute<int32>::CreateLambda([this]() { return GetEditingTrackIndex(); }))
        .DefaultNoteVelocity(TAttribute<int32>::CreateLambda([this]() { return GetDefaultNoteVelocity(); }))
        .DefaultNoteDurationTicks(TAttribute<int32>::CreateLambda([this]() { return GetDefaultNoteDurationTicks(); }))
        .GridSubdivision(TAttribute<EMidiClockSubdivisionQuantization>::CreateLambda([this]() { return GetGridSubdivision(); }))
        .NoteSnapping(TAttribute<EMidiClockSubdivisionQuantization>::CreateLambda([this]() { return GetNoteSnapping(); }))
        .NoteDuration(TAttribute<EMidiClockSubdivisionQuantization>::CreateLambda([this]() { return GetNoteDuration(); }))
        .bIsEditable(bIsMutable);

    // Bind the delete delegate
    PianorollWidget->OnDeleteSelectedNotes.BindUObject(this, &UMidiPianoroll::DeleteSelectedNotes);

    // Bind the notes modified delegate for painting/moving
    PianorollWidget->OnNotesModified.BindLambda([this](const TArray<FNotesEditCallbackData>& Edits)
    {
        UMutableMidiFile* MutableFile = Cast<UMutableMidiFile>(LinkedMidiFile);
        if (MutableFile && Edits.Num() > 0)
        {
            MutableFile->ModifyNotes(Edits);
            // Refresh the display
            SetMidiFile(LinkedMidiFile);
        }
    });

    return PianorollWidget.ToSharedRef();
}

void UMidiPianoroll::ReleaseSlateResources(bool bReleaseChildren)
{
    Super::ReleaseSlateResources(bReleaseChildren);

    PianorollWidget.Reset();
}

#if WITH_EDITOR

void UMidiPianoroll::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
   
	if(PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UMidiPianoroll, LinkedMidiFile))
    {
		SetMidiFile(LinkedMidiFile);
	}
}

#endif
