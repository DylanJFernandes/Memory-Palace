// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FJsonObject;
class FJsonValue;

/* Graded outcome of one answer part, such as the vertex */
struct FStudyPartResult
{
	/* Sets default values for this part result */
	FStudyPartResult();

	/* Identifier of the part, for example "vertex" */
	FString Id;

	/* Name of the part shown to the student */
	FString Label;

	/* Whether the student's answer for this part is correct */
	bool bIsCorrect;

	/* Answer the student wrote, empty if they wrote none */
	FString StudentAnswer;

	/* Correct answer written as text */
	FString CorrectAnswer;
};

/* Numbers needed to draw the correct curve and the student's */
struct FStudyVisual
{
	/* Sets default values for this visual */
	FStudyVisual();

	/* Kind of shape to draw, for example "parabola" */
	FString Type;

	/* Parameters of the correct shape, such as a, h and k */
	TMap<FString, double> Params;

	/* Whether the student gave enough to draw their shape */
	bool bHasStudentParams;

	/* Parameters of the shape built from the student's answers */
	TMap<FString, double> StudentParams;
};

/* Verdict the AI returned for one submitted whiteboard */
struct FStudyReviewResult
{
	/* Sets default values for this result */
	FStudyReviewResult();

	/* Builds a result from the server's result.json object */
	static FStudyReviewResult FromJson(
		const FJsonObject& JsonObject);

	/* Identifier of the problem that was reviewed */
	FString ProblemId;

	/* Identifier of the section the problem belongs to */
	FString SectionId;

	/* Broad subject of the problem, such as Precalculus */
	FString SubjectName;

	/* Readable name of the section, such as Quadratic Functions */
	FString SectionName;

	/* How hard the AI rated the problem; experience scales by it */
	double DifficultyRating;

	/* Seconds between starting the problem and submitting */
	double ElapsedSeconds;

	/* Whether every part is correct */
	bool bIsCorrect;

	/* Whether the server answered without calling the AI */
	bool bIsPlaceholder;

	/* Graded result of each part of the problem */
	TArray<FStudyPartResult> Parts;

	/* Short description of the first mistake, empty if none */
	FString FirstIncorrectStep;

	/* Plain text feedback for the student */
	FString Feedback;

	/* Topics the student appears to be weak in */
	TArray<FString> WeakSubtopics;

	/* Data used to draw the correct and student shapes */
	FStudyVisual Visual;

private:
	/* Reads a text field, returning empty when it is missing */
	static FString ReadString(const FJsonObject& JsonObject,
							  const TCHAR* FieldName);

	/* Reads a true/false field, returning false when missing */
	static bool ReadBool(const FJsonObject& JsonObject,
						 const TCHAR* FieldName);

	/* Reads a number field, returning the fallback when missing */
	static double ReadNumber(const FJsonObject& JsonObject,
							 const TCHAR* FieldName,
							 double FallbackValue);

	/* Reads a list of text values, returning empty when missing */
	static TArray<FString> ReadStringArray(
		const FJsonObject& JsonObject, const TCHAR* FieldName);

	/* Reads every graded part from the parts field */
	static TArray<FStudyPartResult> ParseParts(
		const FJsonObject& JsonObject);

	/* Reads one graded part */
	static FStudyPartResult ParsePart(
		const FJsonObject& PartObject);

	/* Reads the visual field */
	static FStudyVisual ParseVisual(
		const FJsonObject& JsonObject);

	/* Reads an object whose values are all numbers */
	static TMap<FString, double> ParseNumberMap(
		const FJsonObject& JsonObject, const TCHAR* FieldName);
};
