// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HarmonixMidi/MidiFile.h"
#include "MidiFile/MidiNotesData.h"
#include "MutableMidiFile.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnMutableMidiFileChanged);
 
/**
 * 
 */
UCLASS(BlueprintType)
class MIDIEXTENSIONS_API UMutableMidiFile : public UMidiFile
{
	GENERATED_BODY()
	
protected:
	TSharedPtr<FMidiNotesData, ESPMode::ThreadSafe> LinkedMidiData;

public:
	virtual TSharedPtr<Audio::IProxyData> CreateProxyData(const Audio::FProxyDataInitParams& InitParams) override;

	FOnMutableMidiFileChanged OnMutableMidiFileChanged;
};
