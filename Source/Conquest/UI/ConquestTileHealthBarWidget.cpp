#include "Conquest/UI/ConquestTileHealthBarWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UConquestTileHealthBarWidget::SetTileHealth(int32 CurrentHealth, int32 MaxHealth, int32 CombatStrength)
{
	const int32 ClampedMaxHealth = FMath::Max(1, MaxHealth);
	const int32 ClampedCurrentHealth = FMath::Clamp(CurrentHealth, 0, ClampedMaxHealth);

	if (HealthBar)
	{
		HealthBar->SetPercent(static_cast<float>(ClampedCurrentHealth) / static_cast<float>(ClampedMaxHealth));
	}

	if (HealthText)
	{
		HealthText->SetText(FText::Format(
			NSLOCTEXT("Conquest", "TileHealthFormat", "{0}/{1}"),
			FText::AsNumber(ClampedCurrentHealth),
			FText::AsNumber(ClampedMaxHealth)
		));
	}

	if (StrengthText)
	{
		StrengthText->SetText(FText::AsNumber(FMath::Max(0, CombatStrength)));
	}
}
