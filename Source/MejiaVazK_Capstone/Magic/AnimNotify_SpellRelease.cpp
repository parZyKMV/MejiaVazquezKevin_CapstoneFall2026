#include "Magic/AnimNotify_SpellRelease.h"

#include "Magic/SpellComponent.h"

void UAnimNotify_SpellRelease::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (USpellComponent* Spells = USpellComponent::FindFromMesh(MeshComp))
	{
		Spells->ReleaseSpell();
	}
}

FString UAnimNotify_SpellRelease::GetNotifyName_Implementation() const
{
	return TEXT("Spell Release");
}
