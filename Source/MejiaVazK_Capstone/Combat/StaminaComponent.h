#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StaminaComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnStaminaChangedSignature, float, NewStamina, float, MaxStamina, float, Delta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStaminaDepletedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSprintStateChangedSignature, bool, bSprinting);

/**
 * Stamina del personaje: correr y esquivar.
 *
 * - Correr: el input llama SetSprintInput(true/false). Mientras corre de verdad (en el suelo,
 *   moviendose, sin atacar ni esquivar) sube la velocidad y gasta SprintDrainPerSecond.
 *   Al llegar a 0 se corta la carrera y hay que soltar la tecla; no se puede volver a correr
 *   hasta recuperar SprintResumeThreshold.
 * - Esquivar: DodgeComponent llama TrySpend(coste).
 * - Se recupera sola despues de RegenDelay segundos sin gastar.
 *
 * El Tick solo esta encendido cuando hace falta (corriendo, recuperando o tecla pulsada).
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class MEJIAVAZK_CAPSTONE_API UStaminaComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UStaminaComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina", meta = (ClampMin = "1"))
	float MaxStamina = 100.f;

	/** Stamina que se recupera por segundo (cuando ya paso RegenDelay). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina", meta = (ClampMin = "0"))
	float RegenPerSecond = 25.f;

	/** Segundos sin gastar antes de empezar a recuperar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina", meta = (ClampMin = "0"))
	float RegenDelay = 1.f;

	/** Stamina por segundo que gasta correr. 18 = unos 5.5 s de carrera con la barra llena. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina|Sprint", meta = (ClampMin = "0"))
	float SprintDrainPerSecond = 18.f;

	/** Multiplicador de la velocidad de caminata al correr. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina|Sprint", meta = (ClampMin = "1"))
	float SprintSpeedMultiplier = 1.6f;

	/** Tras agotarse, la stamina que hay que recuperar para poder correr otra vez. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina|Sprint", meta = (ClampMin = "0"))
	float SprintResumeThreshold = 20.f;

	/** Velocidad horizontal minima para contar como "corriendo" (evita gastar parado). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina|Sprint", meta = (ClampMin = "0"))
	float MinSprintSpeed = 10.f;

	/** Para la barra del HUD. */
	UPROPERTY(BlueprintAssignable, Category = "Stamina|Events")
	FOnStaminaChangedSignature OnStaminaChanged;

	/** Se llega a 0. */
	UPROPERTY(BlueprintAssignable, Category = "Stamina|Events")
	FOnStaminaDepletedSignature OnStaminaDepleted;

	/** Empieza o termina la carrera (para animacion o sonido). */
	UPROPERTY(BlueprintAssignable, Category = "Stamina|Events")
	FOnSprintStateChangedSignature OnSprintStateChanged;

	/** Conectar a la tecla de correr: Started -> true, Completed -> false. */
	UFUNCTION(BlueprintCallable, Category = "Stamina")
	void SetSprintInput(bool bHeld);

	/** Gasta stamina de golpe (esquive). Devuelve false, sin gastar nada, si no alcanza. */
	UFUNCTION(BlueprintCallable, Category = "Stamina")
	bool TrySpend(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Stamina")
	void Restore(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Stamina")
	void RestoreFull();

	UFUNCTION(BlueprintPure, Category = "Stamina")
	bool HasEnough(float Amount) const { return Stamina >= Amount; }

	UFUNCTION(BlueprintPure, Category = "Stamina")
	float GetStamina() const { return Stamina; }

	UFUNCTION(BlueprintPure, Category = "Stamina")
	float GetMaxStamina() const { return MaxStamina; }

	/** 0 a 1. */
	UFUNCTION(BlueprintPure, Category = "Stamina")
	float GetStaminaPercent() const { return MaxStamina > 0.f ? Stamina / MaxStamina : 0.f; }

	UFUNCTION(BlueprintPure, Category = "Stamina")
	bool IsSprinting() const { return bSprintActive; }

	/** True entre llegar a 0 y recuperar SprintResumeThreshold (no se puede correr). */
	UFUNCTION(BlueprintPure, Category = "Stamina")
	bool IsExhausted() const { return bExhausted; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UFUNCTION()
	void HandleOwnerHealthChanged(float NewHealth, float MaxHealth, float Delta, AActor* DamageCauser);

	void ChangeStamina(float Delta);
	bool ShouldSprint() const;
	void SetSprintActive(bool bActive);

	float Stamina = 100.f;
	float BaseWalkSpeed = 600.f;
	float LastUseTime = -1000.f;

	bool bSprintHeld = false;
	bool bSprintActive = false;
	bool bExhausted = false;
	bool bOwnerDead = false;
};
