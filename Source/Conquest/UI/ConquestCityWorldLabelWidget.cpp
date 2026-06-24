#include "Conquest/UI/ConquestCityWorldLabelWidget.h"

#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Materials/MaterialInterface.h"

void UConquestCityWorldLabelWidget::SetCityLabel(
	const FName& CityName,
	int32 Population,
	int32 CurrentHealth,
	int32 MaxHealth,
	int32 Strength,
	UMaterialInterface* CivilisationThemeMaterial,
	FLinearColor CivilisationThemeColor
)
{
	if (CityNameText)
	{
		CityNameText->SetText(FText::FromName(CityName));
	}

	if (PopulationText)
	{
		PopulationText->SetText(FText::AsNumber(Population));
	}

	if (StrengthText)
	{
		StrengthText->SetText(FText::AsNumber(Strength));
	}

	if (HealthBar)
	{
		const int32 ClampedMaxHealth = FMath::Max(1, MaxHealth);
		const int32 ClampedCurrentHealth = FMath::Clamp(CurrentHealth, 0, ClampedMaxHealth);
		HealthBar->SetPercent(static_cast<float>(ClampedCurrentHealth) / static_cast<float>(ClampedMaxHealth));
		HealthBar->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (ThemeMaterialImage && CivilisationThemeMaterial)
	{
		ThemeMaterialImage->SetBrushFromMaterial(CivilisationThemeMaterial);
	}

	if (ThemeMaterialImage)
	{
		ThemeMaterialImage->SetColorAndOpacity(CivilisationThemeColor);
	}

	OnCivilisationThemeChanged(CivilisationThemeMaterial, CivilisationThemeColor);
}
