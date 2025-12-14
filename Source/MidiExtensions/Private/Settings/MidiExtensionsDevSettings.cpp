// Fill out your copyright notice in the Description page of Project Settings.


#include "Settings/MidiExtensionsDevSettings.h"

UMidiExtensionsDevSettings::UMidiExtensionsDevSettings()
{
	for (int i = 0; i < 16; i++)
	{
		DefaultTrackColors.Add(FLinearColor::IntToDistinctColor(i));
	}
}
