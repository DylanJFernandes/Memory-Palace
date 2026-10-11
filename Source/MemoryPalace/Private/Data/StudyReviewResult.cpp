// Copyright Epic Games, Inc. All Rights Reserved.

#include "Data/StudyReviewResult.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

FStudyPartResult::FStudyPartResult()
	: bIsCorrect{false}
{
}

FStudyVisual::FStudyVisual()
	: bHasStudentParams{false}
{
}

FStudyReviewResult::FStudyReviewResult()
	: DifficultyRating{1.0}
	, ElapsedSeconds{0.0}
	, bIsCorrect{false}
	, bIsPlaceholder{false}
{
}

FStudyReviewResult FStudyReviewResult::FromJson(
	const FJsonObject& JsonObject)
{
	FStudyReviewResult Result;
	Result.ProblemId = ReadString(JsonObject, TEXT("problemId"));
	Result.SectionId = ReadString(JsonObject, TEXT("sectionId"));
	Result.SubjectName = ReadString(JsonObject, TEXT("subjectName"));
	Result.SectionName = ReadString(JsonObject, TEXT("sectionName"));
	Result.DifficultyRating = ReadNumber(
		JsonObject, TEXT("difficultyRating"), 1.0);
	Result.ElapsedSeconds =
		ReadNumber(JsonObject, TEXT("elapsedSeconds"), 0.0);
	Result.bIsCorrect = ReadBool(JsonObject, TEXT("isCorrect"));
	Result.bIsPlaceholder =
		ReadBool(JsonObject, TEXT("isPlaceholder"));
	Result.Parts = ParseParts(JsonObject);
	Result.FirstIncorrectStep =
		ReadString(JsonObject, TEXT("firstIncorrectStep"));
	Result.Feedback = ReadString(JsonObject, TEXT("feedback"));
	Result.WeakSubtopics =
		ReadStringArray(JsonObject, TEXT("weakSubtopics"));
	Result.Visual = ParseVisual(JsonObject);
	return Result;
}

TArray<FStudyPartResult> FStudyReviewResult::ParseParts(
	const FJsonObject& JsonObject)
{
	TArray<FStudyPartResult> Parts;
	const TArray<TSharedPtr<FJsonValue>>* PartValues{nullptr};
	if (!JsonObject.TryGetArrayField(TEXT("parts"), PartValues))
	{
		return Parts;
	}
	for (const TSharedPtr<FJsonValue>& PartValue : *PartValues)
	{
		const TSharedPtr<FJsonObject> PartObject{
			PartValue->AsObject()};
		if (PartObject.IsValid())
		{
			Parts.Add(ParsePart(*PartObject));
		}
	}
	return Parts;
}

FStudyPartResult FStudyReviewResult::ParsePart(
	const FJsonObject& PartObject)
{
	FStudyPartResult Part;
	Part.Id = ReadString(PartObject, TEXT("id"));
	Part.Label = ReadString(PartObject, TEXT("label"));
	Part.bIsCorrect = ReadBool(PartObject, TEXT("isCorrect"));
	Part.StudentAnswer =
		ReadString(PartObject, TEXT("studentAnswer"));
	Part.CorrectAnswer =
		ReadString(PartObject, TEXT("correctAnswer"));
	return Part;
}

FStudyVisual FStudyReviewResult::ParseVisual(
	const FJsonObject& JsonObject)
{
	FStudyVisual Visual;
	const TSharedPtr<FJsonObject>* VisualObject{nullptr};
	if (!JsonObject.TryGetObjectField(TEXT("visual"), VisualObject))
	{
		return Visual;
	}
	const FJsonObject& VisualFields{**VisualObject};
	Visual.Type = ReadString(VisualFields, TEXT("type"));
	Visual.Params = ParseNumberMap(VisualFields, TEXT("params"));
	Visual.bHasStudentParams = VisualFields.HasTypedField<
		EJson::Object>(TEXT("studentParams"));
	Visual.StudentParams =
		ParseNumberMap(VisualFields, TEXT("studentParams"));
	return Visual;
}

TMap<FString, double> FStudyReviewResult::ParseNumberMap(
	const FJsonObject& JsonObject, const TCHAR* FieldName)
{
	TMap<FString, double> Numbers;
	const TSharedPtr<FJsonObject>* NumberObject{nullptr};
	if (!JsonObject.TryGetObjectField(FieldName, NumberObject))
	{
		return Numbers;
	}
	for (const auto& Field : (*NumberObject)->Values)
	{
		double Number{0.0};
		if (Field.Value->TryGetNumber(Number))
		{
			Numbers.Add(FString{Field.Key}, Number);
		}
	}
	return Numbers;
}

FString FStudyReviewResult::ReadString(
	const FJsonObject& JsonObject, const TCHAR* FieldName)
{
	FString Value;
	JsonObject.TryGetStringField(FieldName, Value);
	return Value;
}

bool FStudyReviewResult::ReadBool(
	const FJsonObject& JsonObject, const TCHAR* FieldName)
{
	bool Value{false};
	JsonObject.TryGetBoolField(FieldName, Value);
	return Value;
}

double FStudyReviewResult::ReadNumber(
	const FJsonObject& JsonObject, const TCHAR* FieldName,
	double FallbackValue)
{
	double Value{FallbackValue};
	JsonObject.TryGetNumberField(FieldName, Value);
	return Value;
}

TArray<FString> FStudyReviewResult::ReadStringArray(
	const FJsonObject& JsonObject, const TCHAR* FieldName)
{
	TArray<FString> Values;
	JsonObject.TryGetStringArrayField(FieldName, Values);
	return Values;
}
