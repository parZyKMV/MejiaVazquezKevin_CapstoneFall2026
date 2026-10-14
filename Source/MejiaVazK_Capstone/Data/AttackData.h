#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AttackData.generated.h"

class UAnimMontage;

/**
 * Datos de UN ataque del combo (montage, dano, aturdimiento, alcance).
 * Se crea como Data Asset desde el editor: no hace falta recompilar para ajustarlo.
 */
UCLASS(BlueprintType)
class MEJIAVAZK_CAPSTONE_API UAttackData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Montage de este ataque. Cada ataque debe usar su propio montage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	TObjectPtr<UAnimMontage> Montage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0.1"))
	float PlayRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0.0"))
	float Damage = 20.f;

	/** Segundos que el enemigo queda inmovilizado al recibir este golpe (se renueva con cada golpe). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0.0"))
	float StunDuration = 2.5f;

	/** Distancia hacia adelante (desde el centro del personaje) que recorre el trace. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Trace", meta = (ClampMin = "0.0"))
	float TraceRange = 150.f;

	/** Radio de la esfera del trace. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Trace", meta = (ClampMin = "1.0"))
	float TraceRadius = 80.f;
};