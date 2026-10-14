#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/AttackData.h"
#include "ComboData.generated.h"

/**
 * Un combo completo: la lista de ataques en orden, para suelo y para aire.
 */
UCLASS(BlueprintType)
class MEJIAVAZK_CAPSTONE_API UComboData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Ataques del combo en el suelo, en orden (ataque 1, 2, 3...). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
	TArray<TObjectPtr<UAttackData>> GroundAttacks;

	/** Ataques del combo en el aire. Si esta vacio, se usa el combo de suelo tambien en el aire. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
	TArray<TObjectPtr<UAttackData>> AirAttacks;

	/** Segundos de espera despues del ultimo golpe del combo (evita spamear). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo", meta = (ClampMin = "0.0"))
	float CooldownDuration = 0.6f;
};