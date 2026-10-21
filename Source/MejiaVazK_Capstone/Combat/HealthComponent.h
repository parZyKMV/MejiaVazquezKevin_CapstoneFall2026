#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

// NewHealth, MaxHealth, Delta (negativo = dano, positivo = curacion), quien lo causo
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHealthChangedSignature, float, NewHealth, float, MaxHealth, float, Delta, AActor*, DamageCauser);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDeathSignature, AActor*, Victim, AActor*, Killer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHitStunSignature);

/**
 * Vida, dano y aturdimiento (hit stun). Lo usan jugador, enemigos y props golpeables.
 * Escucha el dano estandar de Unreal (UGameplayStatics::ApplyDamage / TakeDamage).
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class MEJIAVAZK_CAPSTONE_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHealthComponent();

	/** Vida maxima. Se aplica al iniciar el juego. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health", meta = (ClampMin = "1.0"))
	float MaxHealth = 100.f;

	UPROPERTY(BlueprintAssignable, Category = "Health|Events")
	FOnHealthChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Health|Events")
	FOnDeathSignature OnDeath;

	UPROPERTY(BlueprintAssignable, Category = "Health|Events")
	FOnHitStunSignature OnStunStarted;

	UPROPERTY(BlueprintAssignable, Category = "Health|Events")
	FOnHitStunSignature OnStunEnded;

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealthPercent() const { return MaxHealth > 0.f ? CurrentHealth / MaxHealth : 0.f; }

	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsDead() const { return bIsDead; }

	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsStunned() const { return bIsStunned; }

	UFUNCTION(BlueprintCallable, Category = "Health")
	void Heal(float Amount);

	/** Revive al dueno con HealthPercent (0-1) de su vida maxima. Quita el estado de muerte y el aturdimiento. */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void Revive(float HealthPercent = 1.f);

	/** Aturde al dueno durante Duration segundos. Si ya esta aturdido, renueva el tiempo. */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void ApplyHitStun(float Duration);

	/** Termina el aturdimiento de inmediato (por ejemplo, cuando el combo se reinicia). */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void ReleaseFromStun();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Health")
	float CurrentHealth = 0.f;

private:
	UFUNCTION()
	void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

	void Die(AActor* Killer);

	bool bIsDead = false;
	bool bIsStunned = false;
	FTimerHandle StunTimerHandle;
};