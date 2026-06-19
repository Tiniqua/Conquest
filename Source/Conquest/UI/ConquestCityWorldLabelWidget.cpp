#include "Conquest/UI/ConquestCityWorldLabelWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Materials/MaterialInterface.h"

void UConquestCityWorldLabelWidget::SetCityLabel(
	const FName& CityName,
	int32 Population,
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
