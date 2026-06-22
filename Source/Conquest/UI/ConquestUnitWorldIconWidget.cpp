#include "Conquest/UI/ConquestUnitWorldIconWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Materials/MaterialInterface.h"

void UConquestUnitWorldIconWidget::SetUnitIcon(
	const FText& UnitName,
	const FText& CivilisationName,
	FLinearColor UnitDisplayColor,
	UMaterialInterface* UnitIconMaterial
)
{
	if (UnitNameText)
	{
		UnitNameText->SetText(UnitName);
	}

	if (CivilisationNameText)
	{
		CivilisationNameText->SetText(CivilisationName);
	}

	if (UnitIconImage && UnitIconMaterial)
	{
		UnitIconImage->SetBrushFromMaterial(UnitIconMaterial);
	}

	if (UnitIconImage)
	{
		UnitIconImage->SetColorAndOpacity(UnitDisplayColor);
	}

	OnUnitIconChanged(UnitName, CivilisationName, UnitDisplayColor, UnitIconMaterial);
}
