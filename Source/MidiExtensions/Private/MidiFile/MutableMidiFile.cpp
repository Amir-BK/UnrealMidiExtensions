// Copyright notice in the Description page of Project Settings.

#include "MidiFile/MutableMidiFile.h"
#include "CoreMinimal.h"
#include "HarmonixMidi/MidiTrack.h"
#include "HarmonixMidi/MidiEvent.h"
#include "HarmonixMidi/MidiMsg.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"


TSharedPtr<Audio::IProxyData> UMutableMidiFile::CreateProxyData(const Audio::FProxyDataInitParams& InitParams)
{
    LinkedMidiData = FMidiNotesData::BuildFromMidiFile(this);
	
	return UMidiFile::CreateProxyData(InitParams);
}


void UMutableMidiFile::InitializeFromMidiFile(UMidiFile* SourceFile)
{
	if (!SourceFile)
	{
		return;
	}

	// Copy the base file's MIDI data using the proxy system
	Audio::FProxyDataInitParams InitParams{ TEXT("MutableMidiFile") };
	auto ProxyData = SourceFile->CreateProxyData(InitParams);
	TheMidiData = *StaticCastSharedPtr<FMidiFileProxy>(ProxyData)->GetMidiFile();

	// Sort all tracks to ensure consistency
	SortAllTracks();

	// Build linked MIDI data for editing
	LinkedMidiData = FMidiNotesData::BuildFromMidiFile(this);

	// Invalidate renderable copy to force regeneration
	RenderableCopyOfMidiFileData = nullptr;
	ProxyData.Reset();
}

void UMutableMidiFile::ModifyNotes(const TArray<FNotesEditCallbackData>& NotesEdits, FOnNotesEdit OnNotesEditComplete)
{
	if (NotesEdits.IsEmpty() || !LinkedMidiData.IsValid())
	{
		return;
	}

	// Group edits by track for efficiency
	TMap<int32, TArray<const FNotesEditCallbackData*>> EditsByTrack;
	for (const FNotesEditCallbackData& Edit : NotesEdits)
	{
		EditsByTrack.FindOrAdd(Edit.TrackIndex).Add(&Edit);
	}

	// Process each track's edits
	for (auto& [TrackIndex, Edits] : EditsByTrack)
	{
		if (!LinkedMidiData->Tracks.IsValidIndex(TrackIndex))
		{
			UE_LOG(LogTemp, Warning, TEXT("ModifyNotes: Invalid track index %d"), TrackIndex);
			continue;
		}

		FMidiNotesTrack& NotesTrack = LinkedMidiData->Tracks[TrackIndex];
		FMidiTrack* MidiTrack = GetTrack(NotesTrack.TrackIndex);
		if (!MidiTrack)
		{
			UE_LOG(LogTemp, Warning, TEXT("ModifyNotes: Could not find MIDI track for notes track %d"), TrackIndex);
			continue;
		}

		// Separate deletions and modifications/additions
		TArray<const FNotesEditCallbackData*> Deletions;
		TArray<const FNotesEditCallbackData*> Modifications;

		for (const FNotesEditCallbackData* Edit : Edits)
		{
			if (Edit->bDelete)
			{
				Deletions.Add(Edit);
			}
			else
			{
				Modifications.Add(Edit);
			}
		}

		// Sort deletions by NoteIndex descending to preserve indices during removal
		Algo::Sort(Deletions, [](const FNotesEditCallbackData* A, const FNotesEditCallbackData* B)
		{
			return A->NoteIndex > B->NoteIndex;
		});

		// Process deletions first (in reverse index order)
		for (const FNotesEditCallbackData* Delete : Deletions)
		{
			if (NotesTrack.Notes.IsValidIndex(Delete->NoteIndex))
			{
				const FLinkedMidiNote& NoteToDelete = NotesTrack.Notes[Delete->NoteIndex];
				
				// Find and remove the MIDI events (note-on and note-off)
				RemoveNoteEventsFromTrack(MidiTrack, NoteToDelete, NotesTrack.ChannelIndex);
				
				// Remove from linked data
				NotesTrack.Notes.RemoveAt(Delete->NoteIndex);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("ModifyNotes: Invalid note index %d for deletion in track %d"), Delete->NoteIndex, TrackIndex);
			}
		}

		// Process modifications/additions
		for (const FNotesEditCallbackData* Mod : Modifications)
		{
			if (NotesTrack.Notes.IsValidIndex(Mod->NoteIndex))
			{
				// Modification: remove old events, update data, add new events
				const FLinkedMidiNote& OldNote = NotesTrack.Notes[Mod->NoteIndex];
				RemoveNoteEventsFromTrack(MidiTrack, OldNote, NotesTrack.ChannelIndex);
				
				// Update linked data
				NotesTrack.Notes[Mod->NoteIndex] = Mod->NoteData;
				
				// Add new MIDI events
				AddNoteEventsToTrack(MidiTrack, Mod->NoteData, NotesTrack.ChannelIndex);
			}
			else
			{
				// Addition: add new note to linked data and MIDI track
				NotesTrack.Notes.Add(Mod->NoteData);
				AddNoteEventsToTrack(MidiTrack, Mod->NoteData, NotesTrack.ChannelIndex);
			}
		}
	}

	// Sort all tracks after batch modifications
	SortAllTracks();

	// Invalidate renderable copy to force regeneration
	RenderableCopyOfMidiFileData = nullptr;

	// Broadcast change notification
	OnMutableMidiFileChanged.Broadcast();
	
	// Execute callback if bound
	if (OnNotesEditComplete.IsBound())
	{
		OnNotesEditComplete.Execute(NotesEdits);
	}
}

void UMutableMidiFile::RemoveNoteEventsFromTrack(FMidiTrack* Track, const FLinkedMidiNote& Note, int32 Channel)
{
	if (!Track)
	{
		return;
	}

	TArray<FMidiEvent>& Events = Track->GetRawEvents();
	
	// Find and remove note-off event first (higher index typically)
	// Search from end to beginning for efficiency
	int32 NoteOffIndex = INDEX_NONE;
	int32 NoteOnIndex = INDEX_NONE;
	
	for (int32 i = Events.Num() - 1; i >= 0; --i)
	{
		const FMidiEvent& Event = Events[i];
		const FMidiMsg& Msg = Event.GetMsg();
		
		if (Msg.GetStdChannel() != Channel)
		{
			continue;
		}
		
		if (Msg.IsNoteOff() && 
			Event.GetTick() == Note.NoteOffTick && 
			Msg.GetStdData1() == Note.NoteNumber)
		{
			NoteOffIndex = i;
		}
		else if (Msg.IsNoteOn() && 
				 Event.GetTick() == Note.NoteOnTick && 
				 Msg.GetStdData1() == Note.NoteNumber)
		{
			NoteOnIndex = i;
		}
		
		// Found both, can stop searching
		if (NoteOffIndex != INDEX_NONE && NoteOnIndex != INDEX_NONE)
		{
			break;
		}
	}
	
	// Remove in reverse order to preserve indices
	if (NoteOffIndex != INDEX_NONE && NoteOnIndex != INDEX_NONE)
	{
		int32 FirstToRemove = FMath::Max(NoteOffIndex, NoteOnIndex);
		int32 SecondToRemove = FMath::Min(NoteOffIndex, NoteOnIndex);
		
		Events.RemoveAt(FirstToRemove, 1, EAllowShrinking::No);
		Events.RemoveAt(SecondToRemove, 1, EAllowShrinking::No);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("RemoveNoteEventsFromTrack: Could not find note events for note %d at ticks %d-%d"), 
			Note.NoteNumber, Note.NoteOnTick, Note.NoteOffTick);
	}
}

void UMutableMidiFile::AddNoteEventsToTrack(FMidiTrack* Track, const FLinkedMidiNote& Note, int32 Channel)
{
	if (!Track)
	{
		return;
	}

	// Create note-on message
	FMidiMsg NoteOnMsg = FMidiMsg::CreateNoteOn(Channel, Note.NoteNumber, Note.Velocity);
	FMidiEvent NoteOnEvent(Note.NoteOnTick, NoteOnMsg);
	
	// Create note-off message
	FMidiMsg NoteOffMsg = FMidiMsg::CreateNoteOff(Channel, Note.NoteNumber);
	FMidiEvent NoteOffEvent(Note.NoteOffTick, NoteOffMsg);
	
	// Add events to track (will be sorted later)
	Track->AddEvent(NoteOnEvent);
	Track->AddEvent(NoteOffEvent);
}

UMidiFile* UMutableMidiFile::SaveAsAsset(const FString& PackagePath, const FString& AssetName)
{
	if (AssetName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("SaveAsAsset: Asset name cannot be empty"));
		return nullptr;
	}

	// Build the full package path
	FString FullPackagePath = PackagePath;
	if (!FullPackagePath.EndsWith(TEXT("/")))
	{
		FullPackagePath += TEXT("/");
	}
	FullPackagePath += AssetName;

	// Validate the package path
	if (!FPackageName::IsValidLongPackageName(FullPackagePath))
	{
		UE_LOG(LogTemp, Warning, TEXT("SaveAsAsset: Invalid package path '%s'"), *FullPackagePath);
		return nullptr;
	}

	// Create the package
	UPackage* Package = CreatePackage(*FullPackagePath);
	if (!Package)
	{
		UE_LOG(LogTemp, Warning, TEXT("SaveAsAsset: Failed to create package '%s'"), *FullPackagePath);
		return nullptr;
	}

	Package->FullyLoad();

	// Create a new UMutableMidiFile to save as the asset (keeping it mutable for future edits)
	UMutableMidiFile* NewMidiFile = NewObject<UMutableMidiFile>(Package, *AssetName, RF_Public | RF_Standalone);
	if (!NewMidiFile)
	{
		UE_LOG(LogTemp, Warning, TEXT("SaveAsAsset: Failed to create MIDI file object"));
		return nullptr;
	}

	// Copy the MIDI data directly using InitializeFromMidiFile
	NewMidiFile->InitializeFromMidiFile(this);

	// Mark the package as dirty
	Package->MarkPackageDirty();

	// Save the package
	FString PackageFilename = FPackageName::LongPackageNameToFilename(FullPackagePath, FPackageName::GetAssetPackageExtension());
	
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.Error = GError;
	SaveArgs.bWarnOfLongFilename = true;
	
	FSavePackageResultStruct SaveResult = UPackage::Save(Package, NewMidiFile, *PackageFilename, SaveArgs);
	
	if (SaveResult.Result == ESavePackageResult::Success)
	{
		UE_LOG(LogTemp, Log, TEXT("SaveAsAsset: Successfully saved MIDI file to '%s'"), *PackageFilename);
		return NewMidiFile;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("SaveAsAsset: Failed to save package to '%s'"), *PackageFilename);
		return nullptr;
	}
}

