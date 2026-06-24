#include "Conquest/UI/ConquestUnitWorldIconWidget.h"

#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Materials/MaterialInterface.h"

void UConquestUnitWorldIconWidget::SetUnitIcon(
	const FText& UnitName,
	const FText& CivilisationName,
	FLinearColor UnitDisplayColor,
	UMaterialInterface* UnitIconMaterial,
	int32 CurrentHealth,
	int32 MaxHealth,
	int32 AttackValue
)
{
	const int32 ClampedMaxHealth = FMath::Max(1, MaxHealth);
	const int32 ClampedCurrentHealth = FMath::Clamp(CurrentHealth, 0, ClampedMaxHealth);

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

	if (HealthBar)
	{
		HealthBar->SetPercent(static_cast<float>(ClampedCurrentHealth) / static_cast<float>(ClampedMaxHealth));
		HealthBar->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (AttackText)
	{
		AttackText->SetText(FText::Format(
			NSLOCTEXT("Conquest", "UnitWorldIconAttackValue", "ATK {0}"),
			FText::AsNumber(FMath::Max(1, AttackValue))
		));
		AttackText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	OnUnitIconChanged(
		UnitName,
		CivilisationName,
		UnitDisplayColor,
		UnitIconMaterial,
		ClampedCurrentHealth,
		ClampedMaxHealth,
		FMath::Max(1, AttackValue)
	);
}
