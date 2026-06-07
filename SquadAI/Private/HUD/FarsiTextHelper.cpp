// FarsiTextHelper.cpp — Persian text utilities
#include "HUD/FarsiTextHelper.h"

// Persian digits U+06F0..U+06F9: ۰ ۱ ۲ ۳ ۴ ۵ ۶ ۷ ۸ ۹
static const TCHAR PersianDigits[] = {
	0x06F0, 0x06F1, 0x06F2, 0x06F3, 0x06F4,
	0x06F5, 0x06F6, 0x06F7, 0x06F8, 0x06F9
};

FString UFarsiTextHelper::ToPersianDigits(const FString& Input)
{
	FString Result = Input;
	for (int32 i = 0; i < Result.Len(); ++i)
	{
		const TCHAR Ch = Result[i];
		if (Ch >= TEXT('0') && Ch <= TEXT('9'))
		{
			Result[i] = PersianDigits[Ch - TEXT('0')];
		}
	}
	return Result;
}

FString UFarsiTextHelper::IntToPersian(int32 Value)
{
	return ToPersianDigits(FString::FromInt(Value));
}

FString UFarsiTextHelper::FormatTimerPersian(float TotalSeconds)
{
	const int32 Min = FMath::FloorToInt(TotalSeconds / 60.f);
	const int32 Sec = FMath::FloorToInt(FMath::Fmod(TotalSeconds, 60.f));
	return ToPersianDigits(FString::Printf(TEXT("%d:%02d"), Min, Sec));
}

FText UFarsiTextHelper::FormatWavePersian(int32 Current, int32 Total)
{
	// موج X از Y
	return FText::FromString(FString::Printf(
		TEXT("\x0645\x0648\x062C %s \x0627\x0632 %s"),
		*IntToPersian(Current), *IntToPersian(Total)));
}

FText UFarsiTextHelper::FormatKillsPersian(int32 Kills)
{
	// کشته: X
	return FText::FromString(FString::Printf(
		TEXT("\x06A9\x0634\x062A\x0647: %s"), *IntToPersian(Kills)));
}

FText UFarsiTextHelper::FormatRemainingPersian(int32 Remaining)
{
	// باقی‌مانده: X  (with zero-width non-joiner)
	return FText::FromString(FString::Printf(
		TEXT("\x0628\x0627\x0642\x06CC\x200C\x0645\x0627\x0646\x062F\x0647: %s"),
		*IntToPersian(Remaining)));
}

FString UFarsiTextHelper::GetPersianCardinal(float YawDegrees)
{
	float Y = FMath::Fmod(YawDegrees + 360.f, 360.f);
	if (Y >= 337.5f || Y < 22.5f)   return TEXT("\x0634\x0645\x0627\x0644");       // شمال
	if (Y < 67.5f)                    return TEXT("\x0634\x0645\x0627\x0644\x0634\x0631\x0642\x06CC"); // شمالشرقی
	if (Y < 112.5f)                   return TEXT("\x0634\x0631\x0642");             // شرق
	if (Y < 157.5f)                   return TEXT("\x062C\x0646\x0648\x0628\x0634\x0631\x0642\x06CC"); // جنوبشرقی
	if (Y < 202.5f)                   return TEXT("\x062C\x0646\x0648\x0628");       // جنوب
	if (Y < 247.5f)                   return TEXT("\x062C\x0646\x0648\x0628\x063A\x0631\x0628\x06CC"); // جنوبغربی
	if (Y < 292.5f)                   return TEXT("\x063A\x0631\x0628");             // غرب
	return TEXT("\x0634\x0645\x0627\x0644\x063A\x0631\x0628\x06CC");               // شمالغربی
}

FString UFarsiTextHelper::WrapRTL(const FString& Input)
{
	return FString::Printf(TEXT("\x202B%s\x202C"), *Input);
}
