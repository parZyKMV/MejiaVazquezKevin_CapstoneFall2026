#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LockOnComponent.generated.h"

class USpringArmComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLockTargetChangedSignature, AActor*, NewTarget);

/**
 * Fijado de objetivo (lock-on).
 *
 * ToggleLockOn(): fija al enemigo mas cercano delante de la camara y a la vista, o lo suelta.
 * Mientras esta fijado: la camara sigue al objetivo, el personaje lo mira (strafe)
 * y los hechizos salen hacia el (ver SpellComponent::ReleaseSpell).
 *
 * Se suelta solo si el objetivo muere, se aleja demasiado o el dueno muere.
 * Durante los ataques de melee no toca el giro del personaje (lo maneja CombatComponent).
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class MEJIAVAZK_CAPSTONE_API ULockOnComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULockOnComponent();

	/** Distancia maxima para fijar un objetivo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn", meta = (ClampMin = "100"))
	float SearchRadius = 1500.f;

	/** Si el objetivo se aleja mas que esto, se suelta solo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn", meta = (ClampMin = "100"))
	float BreakDistance = 2200.f;

	/** Mitad del cono delante de la camara en el que se busca (grados). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn", meta = (ClampMin = "5", ClampMax = "180"))
	float SearchHalfAngle = 65.f;

	/** Velocidad con que la camara gira hacia el objetivo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn", meta = (ClampMin = "0.1"))
	float CameraTurnSpeed = 7.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
	float MinPitch = -35.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
	float MaxPitch = 15.f;

	/** Altura sobre el centro del objetivo a la que se apunta (pecho). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
	float AimHeightOffset = 30.f;

	/** Ignora enemigos detras de paredes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
	bool bRequireLineOfSight = true;

	/**
	 * Desplazamiento de la camara mientras hay objetivo (relativo a la camara: X adelante, Y derecha, Z arriba).
	 * Y negativo = camara hacia la izquierda; Z positivo = hacia arriba. Asi el jugador no tapa al enemigo.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|Camera")
	FVector LockCameraSocketOffset = FVector(0.f, -70.f, 70.f);

	/** Distancia extra de la camara mientras hay objetivo (positivo = mas lejos). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|Camera")
	float LockCameraExtraDistance = 0.f;

	/** Rapidez con que la camara se mueve a su nueva posicion y vuelve. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|Camera", meta = (ClampMin = "0.1"))
	float CameraOffsetSpeed = 5.f;

	/** Para el marcador en pantalla: nuevo objetivo, o nulo al soltarlo. */
	UPROPERTY(BlueprintAssignable, Category = "LockOn|Events")
	FOnLockTargetChangedSignature OnTargetChanged;

	UFUNCTION(BlueprintCallable, Category = "LockOn")
	void ToggleLockOn();

	UFUNCTION(BlueprintCallable, Category = "LockOn")
	bool AcquireTarget();

	UFUNCTION(BlueprintCallable, Category = "LockOn")
	void ReleaseTarget();

	UFUNCTION(BlueprintPure, Category = "LockOn")
	AActor* GetTarget() const { return Target.Get(); }

	UFUNCTION(BlueprintPure, Category = "LockOn")
	bool IsLockedOn() const { return Target.IsValid(); }

	/** Punto al que se apunta (el pecho del objetivo). Si no hay objetivo, la posicion del dueno. */
	UFUNCTION(BlueprintPure, Category = "LockOn")
	FVector GetTargetAimLocation() const;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	bool IsValidTarget(const AActor* Candidate) const;
	bool HasLineOfSight(const AActor* Candidate) const;
	void SetLockedMovementMode(bool bLocked);

	/** Mueve la camara hacia su posicion de lock-on (o de vuelta). Devuelve true cuando ya llego. */
	bool UpdateCameraOffset(float DeltaTime, bool bLocked);

	TWeakObjectPtr<AActor> Target;

	TWeakObjectPtr<USpringArmComponent> CachedSpringArm;
	FVector DefaultSocketOffset = FVector::ZeroVector;
	float DefaultArmLength = 400.f;

	/** Valores normales del personaje, para restaurarlos al soltar. */
	bool bDefaultOrientToMovement = true;
	bool bDefaultUseControllerYaw = false;

	/** True mientras el personaje esta en modo "mirar al objetivo". */
	bool bLockedModeApplied = false;
};
