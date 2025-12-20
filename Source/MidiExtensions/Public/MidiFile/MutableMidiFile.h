// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HarmonixMidi/MidiFile.h"
#include "MidiFile/MidiNotesData.h"
#include "MutableMidiFile.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnMutableMidiFileChanged);

struct FNotesEditCallbackData
{
	int32 TrackIndex;
	int32 NoteIndex;
	FLinkedMidiNote NoteData;
	bool bDelete = false;
};

DECLARE_DELEGATE_OneParam(FOnNotesEdit, const TArray<FNotesEditCallbackData>&);
 
/**
 * 
 */
UCLASS(BlueprintType)
class MIDIEXTENSIONS_API UMutableMidiFile : public UMidiFile
{
	GENERATED_BODY()
	
protected:
/** Marker property to ensure proper serialization as MutableMidiFile */
UPROPERTY()
bool bIsMutableMidiFile = true;

TSharedPtr<FMidiNotesData, ESPMode::ThreadSafe> LinkedMidiData;

	/** Removes note-on and note-off events from a MIDI track */
	void RemoveNoteEventsFromTrack(FMidiTrack* Track, const FLinkedMidiNote& Note, int32 Channel);

	/** Adds note-on and note-off events to a MIDI track */
	void AddNoteEventsToTrack(FMidiTrack* Track, const FLinkedMidiNote& Note, int32 Channel);

public:
	virtual TSharedPtr<Audio::IProxyData> CreateProxyData(const Audio::FProxyDataInitParams& InitParams) override;

	void InitializeFromMidiFile(UMidiFile* SourceFile);

	/** 
	 * Batch modify notes in the MIDI file.
	 * Handles additions, modifications, and deletions efficiently.
	 * @param NotesEdits Array of edit operations to perform
	 * @param OnNotesEditComplete Optional callback executed after all edits complete
	 */
	void ModifyNotes(const TArray<FNotesEditCallbackData>& NotesEdits, FOnNotesEdit OnNotesEditComplete = FOnNotesEdit());

	/** Get the linked MIDI data for reading */
	TSharedPtr<FMidiNotesData, ESPMode::ThreadSafe> GetLinkedMidiData() const { return LinkedMidiData; }

	/**
	 * Save this MIDI file as a new asset.
	 * @param PackagePath The content browser path for the new asset (e.g., "/Game/MIDI/")
	 * @param AssetName The name for the new asset
	 * @return The newly created asset, or nullptr if save failed
	 */
	UFUNCTION(BlueprintCallable, Category = "MIDI")
	UMidiFile* SaveAsAsset(const FString& PackagePath, const FString& AssetName);

	FOnMutableMidiFileChanged OnMutableMidiFileChanged;
};
