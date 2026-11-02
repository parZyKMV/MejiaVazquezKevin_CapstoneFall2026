#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpellProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;

/**
 * Proyectil base de los hechizos. Se hace un Blueprint hijo (BP_Fireball) para ponerle
 * la malla, el Niagara y el sonido. El SpellComponent le asigna Damage y StunDuration al lanzarlo.
 *
 * Aplica dano con ApplyDamage a lo primero con HealthComponent que toca (menos a quien lo lanzo)
 * y se destruye al chocar con una pared.
 */
UCLASS()
class MEJIAVAZK_CAPSTONE_API ASpellProjectile : public AActor
{
	GENERATED_BODY()

public:
	ASpellProjectile();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell", meta = (ExposeOnSpawn = "true"))
	float Damage = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell", meta = (ExposeOnSpawn = "true"))
	float StunDuration = 0.5f;

	/** Se llama al impactar (con un enemigo o una pared). Pon aqui el efecto y el sonido en Blueprint. HitActor es nulo si fue una pared. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Spell")
	void OnImpact(AActor* HitActor, const FVector& ImpactLocation);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spell")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spell")
	TObjectPtr<UProjectileMovementComponent> Movement;

	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	bool bImpacted = false;
};
