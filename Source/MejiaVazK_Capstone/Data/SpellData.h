#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SpellData.generated.h"

class UAnimMontage;
class UTexture2D;
class ASpellProjectile;

UENUM(BlueprintType)
enum class ESpellElement : uint8
{
	Fire UMETA(DisplayName = "Fire"),
	Water UMETA(DisplayName = "Water"),
	Wind UMETA(DisplayName = "Wind")
};

/**
 * Un hechizo definido por datos (igual que AttackData para el melee).
 * Para crear uno nuevo: Data Asset de esta clase, sin codigo.
 * Los niveles de mejora (1-3) se agregan mas adelante.
 */
UCLASS(BlueprintType)
class MEJIAVAZK_CAPSTONE_API USpellData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell")
	ESpellElement Element = ESpellElement::Fire;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell|Cost", meta = (ClampMin = "0"))
	float ManaCost = 20.f;

	/** Segundos antes de poder volver a lanzarlo. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell|Cost", meta = (ClampMin = "0"))
	float Cooldown = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell|Effect", meta = (ClampMin = "0"))
	float Damage = 25.f;

	/** Segundos que aturde al enemigo golpeado. 0 = no aturde. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell|Effect", meta = (ClampMin = "0"))
	float StunDuration = 0.5f;

	/** Blueprint del proyectil (hijo de SpellProjectile). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell|Effect")
	TSubclassOf<ASpellProjectile> ProjectileClass;

	/** Animacion de lanzamiento. Marca con el notify "Spell Release" el momento en que sale el hechizo. Si esta vacio, sale al instante. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell|Animation")
	TObjectPtr<UAnimMontage> CastMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell|Animation", meta = (ClampMin = "0.1"))
	float PlayRate = 1.f;

	/** Socket del que sale el proyectil. Si el personaje no lo tiene, sale delante del pecho. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell|Animation")
	FName SpawnSocket = TEXT("hand_r");
};
