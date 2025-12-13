// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateWidgetStyle.h"
#include "Styling/SlateWidgetStyleContainerBase.h"

#include "MidiPianorollWidgetStyle.generated.h"

class UMidiFile;

UENUM(BlueprintType)
enum class EMidiTrackTimeMode : uint8
{
	TickLinear UMETA(DisplayName = "Tick Linear"),
	TimeLinear UMETA(DisplayName = "Time Linear"),
};

USTRUCT()
struct MIDIWIDGETS_API FMidiTrackVisualizationData
{
	GENERATED_BODY()

	FMidiTrackVisualizationData()
		: bIsVisible(true)
		, TrackColor(FLinearColor::White)
	{}

	/** Whether this track is visible in the pianoroll */
	UPROPERTY(EditAnywhere, Category = "Appearance")
	bool bIsVisible;

	/** The color to use when rendering notes for this track */
	UPROPERTY(EditAnywhere, Category = "Appearance")
	FLinearColor TrackColor;

	bool operator==(const FMidiTrackVisualizationData& Other) const
	{
		return bIsVisible == Other.bIsVisible && TrackColor == Other.TrackColor;
	}

	bool operator!=(const FMidiTrackVisualizationData& Other) const
	{
		return !(*this == Other);
	}
};

USTRUCT(BlueprintType)
struct MIDIWIDGETS_API FMidiFileVisualizationData
{
	GENERATED_BODY()

	UPROPERTY(EditFixedSize, EditAnywhere, Category = "Appearance")
	TArray<FMidiTrackVisualizationData> TrackVisualizations;

	UPROPERTY(BlueprintReadWrite,EditAnywhere, Category = "Appearance")
	EMidiTrackTimeMode TimeDisplayMode = EMidiTrackTimeMode::TimeLinear;

	static FMidiFileVisualizationData BuildFromMidiFile(UMidiFile* MidiFile);

	bool operator==(const FMidiFileVisualizationData& Other) const
	{
		return TrackVisualizations == Other.TrackVisualizations && TimeDisplayMode == Other.TimeDisplayMode;
	}

	bool operator!=(const FMidiFileVisualizationData& Other) const
	{
		return !(*this == Other);
	}


};

/**
 * 
 */
USTRUCT()
struct MIDIWIDGETS_API FMidiPianorollStyle : public FSlateWidgetStyle
{
	GENERATED_BODY()

	FMidiPianorollStyle();
	virtual ~FMidiPianorollStyle();

	// FSlateWidgetStyle
	virtual void GetResources(TArray<const FSlateBrush*>& OutBrushes) const override;
	static const FName TypeName;
	virtual const FName GetTypeName() const override { return TypeName; };
	static const FMidiPianorollStyle& GetDefault();

	UPROPERTY(EditAnywhere, Category = "Appearance")
	FSlateBrush GridBrush;

	UPROPERTY(EditAnywhere, Category = "Appearance")
	FLinearColor GridColor;

	UPROPERTY(EditAnywhere, Category = "Appearance")
	FLinearColor AccidentalGridColor;

	UPROPERTY(EditAnywhere, Category = "Appearance")
	FSlateBrush NoteBrush;

};

/**
 */
UCLASS(hidecategories=Object, MinimalAPI)
class UMidiPianorollWidgetStyle : public USlateWidgetStyleContainerBase
{
	GENERATED_BODY()

public:
	/** The actual data describing the widget appearance. */
	UPROPERTY(Category=Appearance, EditAnywhere, meta=(ShowOnlyInnerProperties))
	FMidiPianorollStyle WidgetStyle;

	virtual const struct FSlateWidgetStyle* const GetStyle() const override
	{
		return static_cast< const struct FSlateWidgetStyle* >( &WidgetStyle );
	}
};
