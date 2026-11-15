#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "IgnitableComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIgnitedSignature, AActor*, Igniter);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnExtinguishedSignature);

/**
 * Hace que un actor (antorcha, brasero, barril...) se pueda encender con fuego.
 *
 * Los proyectiles con bIgnites = true (BP_Fireball) llaman Ignite() al chocar con un actor
 * que tenga este componente. El actor decide que se ve: se enlazan OnIgnited / OnExtinguished
 * en su Blueprint (encender llama, luz, sonido...).
 *
 * OJO: este componente hace BeginPlay ANTES que el Blueprint del actor, asi que en el BeginPlay
 * del actor hay que leer IsLit() (no esperar el evento) para poner el estado inicial.
 */
UCLASS(ClassGroup = (Magic), meta = (BlueprintSpawnableComponent))
class MEJIAVAZK_CAPSTONE_API UIgnitableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UIgnitableComponent();

	/** Empieza encendido. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ignitable")
	bool bStartLit = false;

	/** Segundos que se mantiene encendido. 0 = se queda encendido para siempre. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ignitable", meta = (ClampMin = "0"))
	float BurnDuration = 0.f;

	/** Se lanza al encenderse. Igniter es quien lo encendio (puede ser nulo). */
	UPROPERTY(BlueprintAssignable, Category = "Ignitable|Events")
	FOnIgnitedSignature OnIgnited;

	UPROPERTY(BlueprintAssignable, Category = "Ignitable|Events")
	FOnExtinguishedSignature OnExtinguished;

	/** Lo enciende. Devuelve false si ya estaba encendido (si tiene BurnDuration, renueva el tiempo). */
	UFUNCTION(BlueprintCallable, Category = "Ignitable")
	bool Ignite(AActor* Igniter);

	UFUNCTION(BlueprintCallable, Category = "Ignitable")
	void Extinguish();

	UFUNCTION(BlueprintPure, Category = "Ignitable")
	bool IsLit() const { return bLit; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void StartBurnTimer();

	bool bLit = false;
	FTimerHandle BurnTimer;
};
