#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_SpellRelease.generated.h"

/**
 * Colocalo en el montage de lanzamiento, en el fotograma en que el hechizo debe salir de la mano.
 * Llama a SpellComponent::ReleaseSpell().
 */
UCLASS()
class MEJIAVAZK_CAPSTONE_API UAnimNotify_SpellRelease : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
