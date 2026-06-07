// FarsiTextHelper.h — Persian digit conversion, RTL utilities, cardinal directions
// Blueprint-callable function library. Zero-cost static functions.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FarsiTextHelper.generated.h"

UCLASS()
class SQUADAI_API UFarsiTextHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Convert ASCII digits 0-9 to Persian digits ۰-۹
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Farsi")
	static FString ToPersianDigits(const FString& Input);

	// Integer → Persian digit string
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Farsi")
	static FString IntToPersian(int32 Value);

	// Format seconds as Persian timer: ۲:۳۴
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Farsi")
	static FString FormatTimerPersian(float TotalSeconds);

	// "موج ۲ از ۵"
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Farsi")
	static FText FormatWavePersian(int32 Current, int32 Total);

	// "کشته: ۲۳"
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Farsi")
	static FText FormatKillsPersian(int32 Kills);

	// "باقی‌مانده: ۱۵"
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Farsi")
	static FText FormatRemainingPersian(int32 Remaining);

	// Persian cardinal: شمال / جنوب / شرق / غرب
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Farsi")
	static FString GetPersianCardinal(float YawDegrees);

	// Wrap text with RTL embedding marks (U+202B / U+202C)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Farsi")
	static FString WrapRTL(const FString& Input);
};
