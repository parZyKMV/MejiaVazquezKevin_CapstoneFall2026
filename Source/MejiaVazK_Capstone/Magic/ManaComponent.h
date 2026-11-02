#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ManaComponent.generated.h"

// NewMana, MaxMana, Delta (negativo = gasto, positivo = recuperacion)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnManaChangedSignature, float, NewMana, float, MaxMana, float, Delta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnManaDepletedSignature);

/**
 * Mana del dueno: maximo, actual, regeneracion con retraso y gasto.
 * Mismo patron que UHealthComponent. Lo usa el SpellComponent con TrySpend().
 *
 * La regeneracion solo ejecuta Tick cuando hace falta (despues de gastar) y se apaga al llenarse.
 */
UCLASS(ClassGroup = (Magic), meta = (BlueprintSpawnableComponent))
class MEJIAVAZK_CAPSTONE_API UManaComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UManaComponent();

	/** Mana maximo. Se aplica al iniciar el juego. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mana", meta = (ClampMin = "1.0"))
	float MaxMana = 100.f;

	/** Mana que se regenera por segundo. 0 = no se regenera. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mana", meta = (ClampMin = "0.0"))
	float RegenPerSecond = 8.f;

	/** Segundos sin regenerar despues de gastar mana. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mana", meta = (ClampMin = "0.0"))
	float RegenDelay = 1.5f;

	UPROPERTY(BlueprintAssignable, Category = "Mana|Events")
	FOnManaChangedSignature OnManaChanged;

	/** Se lanza cuando el mana llega a 0 por un gasto. */
	UPROPERTY(BlueprintAssignable, Category = "Mana|Events")
	FOnManaDepletedSignature OnManaDepleted;

	UFUNCTION(BlueprintPure, Category = "Mana")
	float GetCurrentMana() const { return CurrentMana; }

	UFUNCTION(BlueprintPure, Category = "Mana")
	float GetManaPercent() const { return MaxMana > 0.f ? CurrentMana / MaxMana : 0.f; }

	UFUNCTION(BlueprintPure, Category = "Mana")
	bool HasEnough(float Cost) const { return CurrentMana + KINDA_SMALL_NUMBER >= Cost; }

	/** Intenta gastar mana. Devuelve false (y no gasta nada) si no alcanza. */
	UFUNCTION(BlueprintCallable, Category = "Mana")
	bool TrySpend(float Cost);

	/** Recupera mana (comida, pociones, reaparecer). No pasa del maximo. */
	UFUNCTION(BlueprintCallable, Category = "Mana")
	void Restore(float Amount);

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Mana")
	float CurrentMana = 0.f;

private:
	void ChangeMana(float Delta);

	/** Momento (tiempo del mundo) en que puede volver a regenerarse. */
	float RegenBlockedUntil = 0.f;
};
